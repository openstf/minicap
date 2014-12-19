#ifndef MINICAP_HPP
#define MINICAP_HPP

#include <cstdint>

class minicap
{
public:
  enum format {
    FORMAT_NONE          = 0x01,
    FORMAT_CUSTOM        = 0x02,
    FORMAT_TRANSLUCENT   = 0x03,
    FORMAT_TRANSPARENT   = 0x04,
    FORMAT_OPAQUE        = 0x05,
    FORMAT_RGBA_8888     = 0x06,
    FORMAT_RGBX_8888     = 0x07,
    FORMAT_RGB_888       = 0x08,
    FORMAT_RGB_565       = 0x09,
    FORMAT_BGRA_8888     = 0x0a,
    FORMAT_RGBA_5551     = 0x0b,
    FORMAT_RGBA_4444     = 0x0c,
    FORMAT_UNKNOWN       = 0x00,
  };

  enum orientation {
    ORIENTATION_0    = 0,
    ORIENTATION_90   = 1,
    ORIENTATION_180  = 2,
    ORIENTATION_270  = 3,
  };

  struct display_info {
    uint32_t width;
    uint32_t height;
    uint8_t orientation;
    float fps;
    float density;
    float xdpi;
    float ydpi;
    bool secure;
    float size;
  };

  virtual
  ~minicap() {}

  virtual int
  update(uint32_t width, uint32_t height) = 0;

  virtual void
  release() = 0;

  virtual void const*
  get_pixels() = 0;

  virtual uint32_t
  get_width() = 0;

  virtual uint32_t
  get_height() = 0;

  virtual uint32_t
  get_stride() = 0;

  virtual uint32_t
  get_bpp() = 0;

  virtual size_t
  get_size() = 0;

  virtual format
  get_format() = 0;

  virtual int32_t
  get_display_id() = 0;

  virtual int
  get_display_info(display_info* info) = 0;
};

minicap*
minicap_create(int32_t display_id);

void
minicap_start_thread_pool();

#endif
