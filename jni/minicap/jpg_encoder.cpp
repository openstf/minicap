#include <stdexcept>

#include "jpg_encoder.hpp"

jpg_encoder::jpg_encoder(uint32_t max_width, uint32_t max_height)
  : m_tjhandle(tjInitCompress()),
    m_subsampling(TJSAMP_420),
    m_encoded_data(NULL),
    m_max_width(max_width),
    m_max_height(max_height)
{
  if (reserve_data() != 0) {
    throw std::runtime_error("Unable to reserve data for JPG");
  }
}

jpg_encoder::~jpg_encoder() {
  tjFree(m_encoded_data);
}

int
jpg_encoder::get_format(capster& cp) {
  switch (cp.get_format()) {
  case minicap::FORMAT_RGBA_8888:
    return TJPF_RGBA;
  case minicap::FORMAT_RGBX_8888:
    return TJPF_RGBX;
  case minicap::FORMAT_RGB_888:
    return TJPF_RGB;
  case minicap::FORMAT_BGRA_8888:
    return TJPF_BGRA;
  default:
    throw std::runtime_error("Unsupported pixel format");
  }
}

int
jpg_encoder::encode(capster& cp, unsigned int quality) {
  return tjCompress2(
    m_tjhandle,
    (unsigned char*) cp.get_pixels(),
    cp.get_width(),
    cp.get_stride() * cp.get_bpp(),
    cp.get_height(),
    get_format(cp),
    &m_encoded_data,
    &m_encoded_size,
    m_subsampling,
    quality,
    TJFLAG_FASTDCT | TJFLAG_NOREALLOC
  );
}

int
jpg_encoder::get_encoded_size() {
  return m_encoded_size;
}

unsigned char*
jpg_encoder::get_encoded_data() {
  return m_encoded_data;
}

int
jpg_encoder::reserve_data() {
  unsigned long max_size;

  tjFree(m_encoded_data);

  max_size = tjBufSize(
    m_max_width,
    m_max_height,
    m_subsampling
  );

  m_encoded_data = tjAlloc(max_size);

  return m_encoded_data == NULL ? 1 : 0;
}
