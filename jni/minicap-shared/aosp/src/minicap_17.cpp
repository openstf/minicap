#include "minicap.hpp"
#include "debug.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>

#include <binder/ProcessState.h>

#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

#include <gui/BufferQueue.h>
#include <gui/CpuConsumer.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include <private/gui/ComposerService.h>

#include <ui/DisplayInfo.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>

#include <utils/Mutex.h>
#include <utils/Condition.h>

using namespace android;

class FrameWaiter: public ConsumerBase::FrameAvailableListener {
public:
  FrameWaiter(): mPendingFrames(0) {
  }

  void waitForFrame() {
    Mutex::Autolock lock(mMutex);
    while (mPendingFrames == 0) {
        mCondition.wait(mMutex);
    }
    mPendingFrames--;
  }

  virtual void
  onFrameAvailable() {
    Mutex::Autolock lock(mMutex);
    mPendingFrames++;
    mCondition.signal();
  }

private:
  int mPendingFrames;
  Mutex mMutex;
  Condition mCondition;
};

class minicap_impl: public minicap
{
public:
  minicap_impl(int32_t display_id)
    : minicap(display_id),
      mRealWidth(0),
      mRealHeight(0),
      mDesiredWidth(0),
      mDesiredHeight(0),
      mDesiredOrientation(0),
      mHaveBuffer(false)
  {
  }

  virtual
  ~minicap_impl()
  {
    release();
  }

  virtual int
  begin_updates()
  {
    MCINFO("Starting updates");
    return create_virtual_display();
  }

  virtual bool
  supports_push()
  {
    return true;
  }

  virtual int
  update()
  {
    if (mHaveBuffer) {
      mConsumer->unlockBuffer(mBuffer);
      mHaveBuffer = false;
    }

    mWaiter->waitForFrame();

    status_t err = mConsumer->lockNextBuffer(&mBuffer);

    if (err != NO_ERROR) {
      MCERROR("Unable to lock next buffer %s", error_name(err));
      return 1;
    }

    mHaveBuffer = true;

    return 0;
  }

  virtual void
  release()
  {
    destroy_virtual_display();
  }

  virtual void const*
  get_pixels()
  {
    return mBuffer.data;
  }

  virtual uint32_t
  get_width()
  {
    return mBuffer.width;
  }

  virtual uint32_t
  get_height()
  {
    return mBuffer.height;
  }

  virtual uint32_t
  get_stride()
  {
    return mBuffer.stride;
  }

  virtual uint32_t
  get_bpp()
  {
    return bytesPerPixel(mBuffer.format);
  }

  virtual size_t
  get_size()
  {
    return mBuffer.stride * mBuffer.height * bytesPerPixel(mBuffer.format);
  }

  virtual format
  get_format()
  {
    switch (mBuffer.format)
    {
    case PIXEL_FORMAT_NONE:
      return FORMAT_NONE;
    case PIXEL_FORMAT_CUSTOM:
      return FORMAT_CUSTOM;
    case PIXEL_FORMAT_TRANSLUCENT:
      return FORMAT_TRANSLUCENT;
    case PIXEL_FORMAT_TRANSPARENT:
      return FORMAT_TRANSPARENT;
    case PIXEL_FORMAT_OPAQUE:
      return FORMAT_OPAQUE;
    case PIXEL_FORMAT_RGBA_8888:
      return FORMAT_RGBA_8888;
    case PIXEL_FORMAT_RGBX_8888:
      return FORMAT_RGBX_8888;
    case PIXEL_FORMAT_RGB_888:
      return FORMAT_RGB_888;
    case PIXEL_FORMAT_RGB_565:
      return FORMAT_RGB_565;
    case PIXEL_FORMAT_BGRA_8888:
      return FORMAT_BGRA_8888;
    case PIXEL_FORMAT_RGBA_5551:
      return FORMAT_RGBA_5551;
    case PIXEL_FORMAT_RGBA_4444:
      return FORMAT_RGBA_4444;
    default:
      return FORMAT_UNKNOWN;
    }
  }

  virtual int32_t
  get_display_id()
  {
    return m_display_id;
  }

  virtual int
  get_display_info(display_info* info)
  {
    sp<IBinder> dpy = SurfaceComposerClient::getBuiltInDisplay(m_display_id);

    DisplayInfo dinfo;
    status_t err = SurfaceComposerClient::getDisplayInfo(dpy, &dinfo);

    if (err != NO_ERROR)
    {
      fprintf(stderr, "SurfaceComposerClient::getDisplayInfo() failed: %s (%d)\n",
        error_name(err), err);
      return 1;
    }

    info->width = dinfo.w;
    info->height = dinfo.h;
    info->orientation = dinfo.orientation;
    info->fps = dinfo.fps;
    info->density = dinfo.density;
    info->xdpi = dinfo.xdpi;
    info->ydpi = dinfo.ydpi;
    info->secure = dinfo.secure;
    info->size = sqrt(pow(dinfo.w / dinfo.xdpi, 2) + pow(dinfo.h / dinfo.ydpi, 2));

    return 0;
  }

  virtual int
  set_real_size(uint32_t width, uint32_t height)
  {
    MCINFO("Setting real size to %dx%d", width, height);

    mRealWidth = width;
    mRealHeight = height;

    return 0;
  }

  virtual int
  set_desired_projection(uint32_t width, uint32_t height, uint8_t orientation)
  {
    MCINFO("Changing desired projection to %dx%d/%d", width, height, orientation);

    mDesiredWidth = width;
    mDesiredHeight = height;
    mDesiredOrientation = orientation;

    return 0;
  }

private:
  uint32_t mRealWidth;
  uint32_t mRealHeight;
  uint32_t mDesiredWidth;
  uint32_t mDesiredHeight;
  uint8_t mDesiredOrientation;
  sp<BufferQueue> mBufferQueue;
  sp<CpuConsumer> mConsumer;
  sp<FrameWaiter> mWaiter;
  sp<IBinder> mVirtualDisplay;
  bool mHaveBuffer;
  CpuConsumer::LockedBuffer mBuffer;

  int
  create_virtual_display()
  {
    // Set up virtual display size.
    Rect layerStackRect(mRealWidth, mRealHeight);
    Rect visibleRect(mDesiredWidth, mDesiredHeight);

    // Create a Surface for the virtual display to write to.
    MCINFO("Creating SurfaceComposerClient");
    sp<SurfaceComposerClient> sc = new SurfaceComposerClient();

    MCINFO("Performing SurfaceComposerClient init check");
    if (sc->initCheck() != NO_ERROR) {
      MCERROR("Unable to initialize SurfaceComposerClient");
      return 1;
    }

    // Create virtual display.
    MCINFO("Creating virtual display");
    mVirtualDisplay = SurfaceComposerClient::createDisplay(
      /* const String8& displayName */  String8("minicap"),
      /* bool secure */                 true
    );

    MCINFO("Creating CPU consumer");
    mConsumer = new CpuConsumer(1);
    mConsumer->setName(String8("minicap"));

    MCINFO("Creating buffer queue");
    mBufferQueue = mConsumer->getBufferQueue();
    mBufferQueue->setDefaultBufferSize(mDesiredWidth, mDesiredHeight);
    mBufferQueue->setDefaultBufferFormat(PIXEL_FORMAT_RGB_888);

    MCINFO("Creating frame waiter");
    mWaiter = new FrameWaiter();
    mConsumer->setFrameAvailableListener(mWaiter);

    MCINFO("Publishing virtual display");
    SurfaceComposerClient::openGlobalTransaction();
    SurfaceComposerClient::setDisplaySurface(mVirtualDisplay, mBufferQueue);
    SurfaceComposerClient::setDisplayProjection(mVirtualDisplay,
      mDesiredOrientation,
      layerStackRect, visibleRect);
    SurfaceComposerClient::setDisplayLayerStack(mVirtualDisplay, 0); // default stack
    SurfaceComposerClient::closeGlobalTransaction();

    return 0;
  }

  void
  destroy_virtual_display()
  {
    MCINFO("Destroying virtual display");

    if (mHaveBuffer) {
      mConsumer->unlockBuffer(mBuffer);
      mHaveBuffer = false;
    }

    mBufferQueue = NULL;
    mConsumer = NULL;
    mWaiter = NULL;
    mVirtualDisplay = NULL;
  }

  const char*
  error_name(int32_t err)
  {
    switch (err)
    {
    case android::NO_ERROR: // also android::OK
      return "NO_ERROR";
    case android::UNKNOWN_ERROR:
      return "UNKNOWN_ERROR";
    case android::NO_MEMORY:
      return "NO_MEMORY";
    case android::INVALID_OPERATION:
      return "INVALID_OPERATION";
    case android::BAD_VALUE:
      return "BAD_VALUE";
    case android::BAD_TYPE:
      return "BAD_TYPE";
    case android::NAME_NOT_FOUND:
      return "NAME_NOT_FOUND";
    case android::PERMISSION_DENIED:
      return "PERMISSION_DENIED";
    case android::NO_INIT:
      return "NO_INIT";
    case android::ALREADY_EXISTS:
      return "ALREADY_EXISTS";
    case android::DEAD_OBJECT: // also android::JPARKS_BROKE_IT
      return "DEAD_OBJECT";
    case android::FAILED_TRANSACTION:
      return "FAILED_TRANSACTION";
    case android::BAD_INDEX:
      return "BAD_INDEX";
    case android::NOT_ENOUGH_DATA:
      return "NOT_ENOUGH_DATA";
    case android::WOULD_BLOCK:
      return "WOULD_BLOCK";
    case android::TIMED_OUT:
      return "TIMED_OUT";
    case android::UNKNOWN_TRANSACTION:
      return "UNKNOWN_TRANSACTION";
    case android::FDS_NOT_ALLOWED:
      return "FDS_NOT_ALLOWED";
    default:
      return "UNMAPPED_ERROR";
    }
  }
};

minicap*
minicap_create(int32_t display_id)
{
  return new minicap_impl(display_id);
}

void
minicap_free(minicap* mc)
{
  delete mc;
}

void
minicap_start_thread_pool()
{
  ProcessState::self()->startThreadPool();
}
