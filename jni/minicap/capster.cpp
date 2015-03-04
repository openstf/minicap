#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>

#include <cmath>
#include <stdexcept>
#include <sstream>

#include "capster.hpp"
#include "util/formatter.hpp"

minicap::display_info capster::get_display_info(uint32_t display_id) {
  minicap::display_info info;

  minicap* mc = minicap_create(display_id);

  int err = mc->get_display_info(&info);

  minicap_free(mc);

  if (err != 0) {
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
    info.orientation = vinfo.rotate;
    info.xdpi = static_cast<float>(vinfo.xres) / static_cast<float>(vinfo.width) * 25.4;
    info.ydpi = static_cast<float>(vinfo.yres) / static_cast<float>(vinfo.height) * 25.4;
    info.size = std::sqrt(
      (static_cast<float>(vinfo.width) * static_cast<float>(vinfo.width)) +
      (static_cast<float>(vinfo.height) * static_cast<float>(vinfo.height))) / 25.4;
    info.density = std::sqrt(
      (static_cast<float>(vinfo.xres) * static_cast<float>(vinfo.xres)) +
      (static_cast<float>(vinfo.yres) * static_cast<float>(vinfo.yres))) / info.size;
    info.secure = false;
    info.fps = 0;

    close(fd);
  }

  return info;
}

capster::capster(uint32_t display_id)
  : minicap(display_id),
    m_minicap(minicap_create(display_id))
{
}

capster::~capster() {
  minicap_free(m_minicap);
}

int
capster::begin_updates()
{
  return m_minicap->begin_updates();
}

bool
capster::supports_push()
{
  return m_minicap->supports_push();
}

int
capster::update()
{
  return m_minicap->update();
}

void
capster::release()
{
  m_minicap->release();
}

void const*
capster::get_pixels()
{
  return m_minicap->get_pixels();
}

uint32_t
capster::get_width()
{
  return m_minicap->get_width();
}

uint32_t
capster::get_height()
{
  return m_minicap->get_height();
}

uint32_t
capster::get_stride()
{
  return m_minicap->get_stride();
}

uint32_t
capster::get_bpp()
{
  return m_minicap->get_bpp();
}

size_t
capster::get_size()
{
  return m_minicap->get_size();
}

minicap::format
capster::get_format()
{
  return m_minicap->get_format();
}

int32_t
capster::get_display_id()
{
  return m_minicap->get_display_id();
}

int
capster::get_display_info(display_info* info)
{
  return m_minicap->get_display_info(info);
}

int
capster::set_real_size(uint32_t width, uint32_t height)
{
  return m_minicap->set_real_size(width, height);
}

int
capster::set_desired_projection(uint32_t width, uint32_t height, uint8_t orientation)
{
  return m_minicap->set_desired_projection(width, height, orientation);
}
