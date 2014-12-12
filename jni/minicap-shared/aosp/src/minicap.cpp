#include "minicap.hpp"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <binder/ProcessState.h>

#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

#if PLATFORM_SDK_VERSION >= 16
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#else
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#endif

#include <ui/PixelFormat.h>

using namespace android;

class minicap_impl: public minicap
{
public:
  minicap_impl(int32_t display_id)
    : m_display_id(display_id),
      m_screenshot()
  {
  }

  virtual int
  update(uint32_t width, uint32_t height)
  {
    #if PLATFORM_SDK_VERSION >= 21
    sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(m_display_id);
    int32_t err = m_screenshot.update(display, Rect(), width, height, false);
    #elif PLATFORM_SDK_VERSION >= 17
    sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(m_display_id);
    int32_t err = m_screenshot.update(display, width, height);
    #else
    int32_t err = m_screenshot.update(width, height);
    #endif

    if (err != NO_ERROR)
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
    m_screenshot.release();
  }

  virtual void const*
  get_pixels()
  {
    return m_screenshot.getPixels();
  }

  virtual uint32_t
  get_width()
  {
    return m_screenshot.getWidth();
  }

  virtual uint32_t
  get_height()
  {
    return m_screenshot.getHeight();
  }

  virtual uint32_t
  get_stride()
  {
    return m_screenshot.getStride();
  }

  virtual uint32_t
  get_bpp()
  {
    return bytesPerPixel(m_screenshot.getFormat());
  }

  virtual size_t
  get_size()
  {
    return m_screenshot.getSize();
  }

  virtual format
  get_format()
  {
    switch (m_screenshot.getFormat())
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

  virtual void
  get_display_info()
  {
    // @todo
  }

private:
  int32_t m_display_id;
  ScreenshotClient m_screenshot;

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
    #if PLATFORM_SDK_VERSION >= 14
    case android::FDS_NOT_ALLOWED:
      return "FDS_NOT_ALLOWED";
    #endif
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
minicap_start_thread_pool()
{
  ProcessState::self()->startThreadPool();
}
