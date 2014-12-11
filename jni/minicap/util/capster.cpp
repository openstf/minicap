#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>

#include <cmath>
#include <stdexcept>
#include <sstream>

#include "capster.hpp"
#include "formatter.hpp"

capster::display_info capster::get_display_info(uint32_t display_id) {
  display_info info;

  char path[64];
  sprintf(path, "/dev/graphics/fb%d", display_id);

  int fd = open(path, O_RDONLY);

  if (fd < 0) {
    throw std::runtime_error(formatter() << "Cannot open " << path << ": " << strerror(errno));
  }

  fb_var_screeninfo vinfo;

  if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
    close(fd);
    throw std::runtime_error(formatter() << "Cannot get FBIOGET_VSCREENINFO of " << path << ": " << strerror(errno));
  }

  info.width = vinfo.xres;
  info.height = vinfo.yres;
  info.xdpi = static_cast<float>(vinfo.xres) / static_cast<float>(vinfo.width) * 25.4;
  info.ydpi = static_cast<float>(vinfo.yres) / static_cast<float>(vinfo.height) * 25.4;
  info.size = std::sqrt(
    (static_cast<float>(vinfo.width) * static_cast<float>(vinfo.width)) +
    (static_cast<float>(vinfo.height) * static_cast<float>(vinfo.height))) / 25.4;
  info.density = std::sqrt(
    (static_cast<float>(vinfo.xres) * static_cast<float>(vinfo.xres)) +
    (static_cast<float>(vinfo.yres) * static_cast<float>(vinfo.yres))) / info.size;

  close(fd);

  return info;
}

capster::capster(uint32_t display_id)
  : m_tjhandle(tjInitCompress()),
    m_minicap(minicap_create(display_id)),
    m_subsampling(TJSAMP_420),
    m_quality(80),
    m_max_quality(100),
    m_data(NULL)
{
  m_minicap->update(0, 0);

  m_max_width = m_minicap->get_width();
  m_max_height = m_minicap->get_height();

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
  m_minicap->update(width, height);

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
