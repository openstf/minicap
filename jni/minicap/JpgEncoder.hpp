#ifndef MINICAP_JPG_ENCODER_HPP
#define MINICAP_JPG_ENCODER_HPP

#include <turbojpeg.h>

#include "Minicap.hpp"

class JpgEncoder {
public:
  JpgEncoder(unsigned int prePadding, unsigned int postPadding);

  ~JpgEncoder();

  bool
  encode(Minicap::Frame* frame, unsigned int quality);

  int
  getEncodedSize();

  unsigned char*
  getEncodedData();

  bool
  reserveData(uint32_t width, uint32_t height);

private:
  tjhandle mTjHandle;
  int mSubsampling;
  unsigned int mPrePadding;
  unsigned int mPostPadding;
  unsigned int mMaxWidth;
  unsigned int mMaxHeight;
  unsigned char* mEncodedData;
  unsigned long mEncodedSize;

  static int
  convertFormat(Minicap::Format format);
};

#endif
