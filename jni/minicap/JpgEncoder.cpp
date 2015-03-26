#include <stdexcept>

#include "JpgEncoder.hpp"
#include "util/debug.h"

JpgEncoder::JpgEncoder(unsigned int prePadding, unsigned int postPadding)
  : mTjHandle(tjInitCompress()),
    mSubsampling(TJSAMP_420),
    mEncodedData(NULL),
    mPrePadding(prePadding),
    mPostPadding(postPadding),
    mMaxWidth(0),
    mMaxHeight(0)
{
}

JpgEncoder::~JpgEncoder() {
  tjFree(mEncodedData);
}

bool
JpgEncoder::encode(Minicap::Frame* frame, unsigned int quality) {
  unsigned char* offset = getEncodedData();

  return 0 == tjCompress2(
    mTjHandle,
    (unsigned char*) frame->data,
    frame->width,
    frame->stride * frame->bpp,
    frame->height,
    convertFormat(frame->format),
    &offset,
    &mEncodedSize,
    mSubsampling,
    quality,
    TJFLAG_FASTDCT | TJFLAG_NOREALLOC
  );
}

int
JpgEncoder::getEncodedSize() {
  return mEncodedSize;
}

unsigned char*
JpgEncoder::getEncodedData() {
  return mEncodedData + mPrePadding;
}

bool
JpgEncoder::reserveData(uint32_t width, uint32_t height) {
  if (width == mMaxWidth && height == mMaxHeight) {
    return 0;
  }

  tjFree(mEncodedData);

  unsigned long maxSize = mPrePadding + mPostPadding + tjBufSize(
    width,
    height,
    mSubsampling
  );

  MCINFO("Allocating %ld bytes for JPG encoder", maxSize);

  mEncodedData = tjAlloc(maxSize);

  if (mEncodedData == NULL) {
    return false;
  }

  mMaxWidth = width;
  mMaxHeight = height;

  return true;
}

int
JpgEncoder::convertFormat(Minicap::Format format) {
  switch (format) {
  case Minicap::FORMAT_RGBA_8888:
    return TJPF_RGBA;
  case Minicap::FORMAT_RGBX_8888:
    return TJPF_RGBX;
  case Minicap::FORMAT_RGB_888:
    return TJPF_RGB;
  case Minicap::FORMAT_BGRA_8888:
    return TJPF_BGRA;
  default:
    throw std::runtime_error("Unsupported pixel format");
  }
}
