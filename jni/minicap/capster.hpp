#ifndef MINICAP_CAPSTER_HPP
#define MINICAP_CAPSTER_HPP

#include <memory>

#include <minicap.hpp>

class capster : public minicap {
public:
  static minicap::display_info
  get_display_info(uint32_t display_id);

  capster(uint32_t display_id);

  ~capster();

  virtual int
  begin_updates();

  virtual bool
  supports_push();

  virtual int
  update();

  virtual void
  release();

  virtual void const*
  get_pixels();

  virtual uint32_t
  get_width();

  virtual uint32_t
  get_height();

  virtual uint32_t
  get_stride();

  virtual uint32_t
  get_bpp();

  virtual size_t
  get_size();

  virtual format
  get_format();

  virtual int32_t
  get_display_id();

  virtual int
  get_display_info(display_info* info);

  virtual int
  set_real_size(uint32_t width, uint32_t height);

  virtual int
  set_desired_projection(uint32_t width, uint32_t height, uint8_t orientation);

private:
  minicap* m_minicap;
};

#endif
