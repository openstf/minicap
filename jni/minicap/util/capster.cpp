#include <stdexcept>

#include "capster.hpp"

capster::capster(uint32_t display_id)
  : m_tjhandle(tjInitCompress()),
    m_minicap(minicap_create(display_id)),
    m_subsampling(TJSAMP_420),
    m_quality(80),
    m_max_quality(100),
    m_data(NULL)
{
  m_minicap->update(0, 0);

  m_max_width = m_minicap->get_width() / 4;
  m_max_height = m_minicap->get_height() / 4;

  switch (m_minicap->get_format()) {
  case minicap::FORMAT_RGBA_8888:
    m_format = TJPF_RGBA;
    break;
  case minicap::FORMAT_RGBX_8888:
    m_format = TJPF_RGBX;
    break;
  case minicap::FORMAT_RGB_888:
    m_format = TJPF_RGB;
    break;
  case minicap::FORMAT_BGRA_8888:
    m_format = TJPF_BGRA;
    break;
  default:
    throw std::runtime_error("Unsupported pixel format");
    break;
  }

  reserve_data();
}

int
capster::capture(unsigned int width, unsigned int height) {
  int ok;
  unsigned long size;

  m_minicap->release();
  m_minicap->update(m_max_width, m_max_height);

  ok = tjCompress2(
    m_tjhandle,
    (unsigned char*) m_minicap->get_pixels(),
    m_minicap->get_width(),
    m_minicap->get_stride() * m_minicap->get_bpp(),
    m_minicap->get_height(),
    m_format,
    &m_data,
    &m_data_size,
    m_subsampling,
    m_quality,
    TJFLAG_FASTDCT | TJFLAG_NOREALLOC
  );

  if (ok != 0) {
    throw std::runtime_error("Conversion failed");
  }
}

int
capster::get_size() {
  return m_data_size;
}

unsigned char*
capster::get_data() {
  return m_data;
}

void
capster::reserve_data() {
  unsigned long max_size;

  tjFree(m_data);

  max_size = tjBufSize(
    m_max_width,
    m_max_height,
    m_subsampling
  );

  m_data = tjAlloc(max_size);
}
