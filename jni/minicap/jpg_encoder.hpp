#ifndef MINICAP_JPG_ENCODER_HPP
#define MINICAP_JPG_ENCODER_HPP

#include <turbojpeg.h>

#include "capster.hpp"

class jpg_encoder {
public:
  jpg_encoder(uint32_t width, uint32_t height);

  ~jpg_encoder();

  int
  encode(capster& capster, unsigned int quality);

  int
  get_encoded_size();

  unsigned char*
  get_encoded_data();

private:
  tjhandle m_tjhandle;
  int m_subsampling;
  unsigned int m_max_width;
  unsigned int m_max_height;
  unsigned char* m_encoded_data;
  unsigned long m_encoded_size;

  int
  reserve_data();

  int
  get_format(capster& capster);
};

#endif
