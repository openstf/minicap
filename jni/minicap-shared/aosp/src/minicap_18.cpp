#include "minicap.hpp"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>

#include <binder/ProcessState.h>

#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include <private/gui/ComposerService.h>

#include <ui/DisplayInfo.h>
#include <ui/PixelFormat.h>

using namespace android;

class minicap_impl: public minicap
{
public:
  minicap_impl(int32_t display_id)
    : m_display_id(display_id),
      m_composer(ComposerService::getComposerService()),
      m_display(SurfaceComposerClient::getBuiltInDisplay(display_id)),
      m_have_buffer(false)
  {
  }

  virtual
  ~minicap_impl()
  {
    release();
  }

  virtual int
  update(uint32_t width, uint32_t height)
  {
    sp<CpuConsumer> cpuConsumer = getCpuConsumer();

    if (m_have_buffer) {
        m_cpu_consumer->unlockBuffer(m_buffer);
        m_have_buffer = false;
    }

    status_t err = m_composer->captureScreen(m_display,
      m_cpu_consumer->getBufferQueue(), width, height, 0, -1UL, true);

    if (err == NO_ERROR)
    {
        err = m_cpu_consumer->lockNextBuffer(&m_buffer);

        if (err == NO_ERROR)
        {
            m_have_buffer = true;
        }
    }
    else
    {
      fprintf(stderr, "SurfaceFlingerClient::update() failed: %s (%d)\n",
        error_name(err), err);
      return 1;
    }

    return 0;
  }

  virtual void
  release()
  {
    if (m_have_buffer) {
        m_cpu_consumer->unlockBuffer(m_buffer);
        memset(&m_buffer, 0, sizeof(m_buffer));
        m_have_buffer = false;
    }

    m_cpu_consumer.clear();
  }

  virtual void const*
  get_pixels()
  {
    return m_buffer.data;
  }

  virtual uint32_t
  get_width()
  {
    return m_buffer.width;
  }

  virtual uint32_t
  get_height()
  {
    return m_buffer.height;
  }

  virtual uint32_t
  get_stride()
  {
    return m_buffer.stride;
  }

  virtual uint32_t
  get_bpp()
  {
    return bytesPerPixel(m_buffer.format);
  }

  virtual size_t
  get_size()
  {
    return m_buffer.stride * m_buffer.height * bytesPerPixel(m_buffer.format);
  }

  virtual format
  get_format()
  {
    switch (m_buffer.format)
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
    DisplayInfo dinfo;
    status_t err = SurfaceComposerClient::getDisplayInfo(m_display, &dinfo);

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

private:
  int32_t m_display_id;
  sp<ISurfaceComposer> m_composer;
  sp<IBinder> m_display;
  mutable sp<CpuConsumer> m_cpu_consumer;
  CpuConsumer::LockedBuffer m_buffer;
  bool m_have_buffer;

  sp<CpuConsumer> getCpuConsumer() const
  {
    if (m_cpu_consumer == NULL)
    {
        m_cpu_consumer = new CpuConsumer(1);
        m_cpu_consumer->setName(String8("ScreenshotClient"));
    }

    return m_cpu_consumer;
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
