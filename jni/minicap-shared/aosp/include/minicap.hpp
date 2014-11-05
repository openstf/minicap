#ifndef MINICAP_HPP
#define MINICAP_HPP

#include <cstdint>

#define MINICAP_FORMAT_NONE          0x01
#define MINICAP_FORMAT_CUSTOM        0x02
#define MINICAP_FORMAT_TRANSLUCENT   0x03
#define MINICAP_FORMAT_TRANSPARENT   0x04
#define MINICAP_FORMAT_OPAQUE        0x05
#define MINICAP_FORMAT_RGBA_8888     0x06
#define MINICAP_FORMAT_RGBX_8888     0x07
#define MINICAP_FORMAT_RGB_888       0x08
#define MINICAP_FORMAT_RGB_565       0x09
#define MINICAP_FORMAT_BGRA_8888     0x0a
#define MINICAP_FORMAT_RGBA_5551     0x0b
#define MINICAP_FORMAT_RGBA_4444     0x0c
#define MINICAP_FORMAT_UNKNOWN       0x00

struct minicap
{
  int32_t display_id;
  void *internal;
};

struct minicap*
minicap_create(int32_t display_id);

int
minicap_update(struct minicap *handle, uint32_t width, uint32_t height);

void
minicap_release(struct minicap *handle);

void const*
minicap_get_pixels(struct minicap *handle);

uint32_t
minicap_get_width(struct minicap *handle);

uint32_t
minicap_get_height(struct minicap *handle);

uint32_t
minicap_get_stride(struct minicap *handle);

uint32_t
minicap_get_bpp(struct minicap *handle);

size_t
minicap_get_size(struct minicap *handle);

int
minicap_get_format(struct minicap *handle);

int32_t
minicap_get_display_id(struct minicap *handle);

void
minicap_get_display_info(struct minicap *handle);

void
minicap_free(struct minicap *handle);

#endif
