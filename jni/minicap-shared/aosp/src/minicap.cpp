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

const char*
minicap_error(int32_t err)
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

struct minicap*
minicap_create(int32_t display_id)
{
  ProcessState::self()->startThreadPool();
  struct minicap* mc = (struct minicap*) calloc(1, sizeof(struct minicap));
  mc->display_id = display_id;
  mc->internal = new ScreenshotClient;
  return mc;
}

int
minicap_update(struct minicap *handle, uint32_t width, uint32_t height)
{
  minicap_release(handle); // Required for Android 4.3+

  #if PLATFORM_SDK_VERSION >= 21
  sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(handle->display_id);
  int32_t err = ((ScreenshotClient*) handle->internal)->update(display, Rect(), width, height, false);
  #elif PLATFORM_SDK_VERSION >= 17
  sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(handle->display_id);
  int32_t err = ((ScreenshotClient*) handle->internal)->update(display, width, height);
  #else
  int32_t err = ((ScreenshotClient*) handle->internal)->update(width, height);
  #endif

  if (err != NO_ERROR)
  {
    fprintf(stderr, "SurfaceFlingerClient::update() failed: %s (%d)\n",
      minicap_error(err), err);
    return 1;
  }

  return 0;
}

void
minicap_release(struct minicap *handle)
{
  ((ScreenshotClient*) handle->internal)->release();
}

void const*
minicap_get_pixels(struct minicap *handle)
{
  return ((ScreenshotClient*) handle->internal)->getPixels();
}

uint32_t
minicap_get_width(struct minicap *handle)
{
  return ((ScreenshotClient*) handle->internal)->getWidth();
}

uint32_t
minicap_get_height(struct minicap *handle)
{
  return ((ScreenshotClient*) handle->internal)->getHeight();
}

uint32_t
minicap_get_stride(struct minicap *handle)
{
  return ((ScreenshotClient*) handle->internal)->getStride();
}

uint32_t
minicap_get_bpp(struct minicap *handle)
{
  return bytesPerPixel(((ScreenshotClient*) handle->internal)->getFormat());
}

size_t
minicap_get_size(struct minicap *handle)
{
  return ((ScreenshotClient*) handle->internal)->getSize();
}

int
minicap_get_format(struct minicap *handle)
{
  switch (((ScreenshotClient*) handle->internal)->getFormat())
  {
  case PIXEL_FORMAT_NONE:
    return MINICAP_FORMAT_NONE;
  case PIXEL_FORMAT_CUSTOM:
    return MINICAP_FORMAT_CUSTOM;
  case PIXEL_FORMAT_TRANSLUCENT:
    return MINICAP_FORMAT_TRANSLUCENT;
  case PIXEL_FORMAT_TRANSPARENT:
    return MINICAP_FORMAT_TRANSPARENT;
  case PIXEL_FORMAT_OPAQUE:
    return MINICAP_FORMAT_OPAQUE;
  case PIXEL_FORMAT_RGBA_8888:
    return MINICAP_FORMAT_RGBA_8888;
  case PIXEL_FORMAT_RGBX_8888:
    return MINICAP_FORMAT_RGBX_8888;
  case PIXEL_FORMAT_RGB_888:
    return MINICAP_FORMAT_RGB_888;
  case PIXEL_FORMAT_RGB_565:
    return MINICAP_FORMAT_RGB_565;
  case PIXEL_FORMAT_BGRA_8888:
    return MINICAP_FORMAT_BGRA_8888;
  case PIXEL_FORMAT_RGBA_5551:
    return MINICAP_FORMAT_RGBA_5551;
  case PIXEL_FORMAT_RGBA_4444:
    return MINICAP_FORMAT_RGBA_4444;
  default:
    return MINICAP_FORMAT_UNKNOWN;
  }
}

int32_t
minicap_get_display_id(struct minicap *handle)
{
  return handle->display_id;
}

void
minicap_get_display_info(struct minicap *handle)
{
  // @todo
}

void
minicap_free(struct minicap *handle)
{
  delete ((ScreenshotClient*) handle->internal);
  free(handle);
}
