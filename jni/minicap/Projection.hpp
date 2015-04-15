#ifndef MINICAP_PROJECTION_HPP
#define MINICAP_PROJECTION_HPP

#include <cmath>
#include <ostream>

class Projection {
public:
  class Parser {
  public:
    Parser(): mState(real_width_start) {
    }

    bool
    parse(Projection& proj, const char* lo, const char* hi) {
      consume: while (lo < hi) {
        switch (mState) {
        case real_width_start:
          if (isDigit(*lo)) {
            proj.realWidth += (*lo - 48);
            mState = real_width_continued;
            lo += 1;
            goto consume;
          }
          return false;
        case real_width_continued:
          if (isDigit(*lo)) {
            proj.realWidth *= 10;
            proj.realWidth += (*lo - 48);
            lo += 1;
            goto consume;
          }
          if (*lo == 'x') {
            mState = real_height_start;
            lo += 1;
            goto consume;
          }
          return false;
        case real_height_start:
          if (isDigit(*lo)) {
            proj.realHeight += (*lo - 48);
            mState = real_height_continued;
            lo += 1;
            goto consume;
          }
          return false;
        case real_height_continued:
          if (isDigit(*lo)) {
            proj.realHeight *= 10;
            proj.realHeight += (*lo - 48);
            lo += 1;
            goto consume;
          }
          if (*lo == '@') {
            mState = virtual_width_start;
            lo += 1;
            goto consume;
          }
          return false;
        case virtual_width_start:
          if (isDigit(*lo)) {
            proj.virtualWidth += (*lo - 48);
            mState = virtual_width_continued;
            lo += 1;
            goto consume;
          }
          return false;
        case virtual_width_continued:
          if (isDigit(*lo)) {
            proj.virtualWidth *= 10;
            proj.virtualWidth += (*lo - 48);
            lo += 1;
            goto consume;
          }
          if (*lo == 'x') {
            mState = virtual_height_start;
            lo += 1;
            goto consume;
          }
          return false;
        case virtual_height_start:
          if (isDigit(*lo)) {
            proj.virtualHeight += (*lo - 48);
            mState = virtual_height_continued;
            lo += 1;
            goto consume;
          }
          return false;
        case virtual_height_continued:
          if (isDigit(*lo)) {
            proj.virtualHeight *= 10;
            proj.virtualHeight += (*lo - 48);
            lo += 1;
            goto consume;
          }
          if (*lo == '/') {
            mState = rotation_start;
            lo += 1;
            goto consume;
          }
          return false;
        case rotation_start:
          if (*lo == '0') {
            proj.rotation = 0;
            mState = satisfied;
            lo += 1;
            goto consume;
          }
          if (*lo == '9') {
            mState = rotation_90_2;
            lo += 1;
            goto consume;
          }
          if (*lo == '1') {
            mState = rotation_180_2;
            lo += 1;
            goto consume;
          }
          if (*lo == '2') {
            mState = rotation_270_2;
            lo += 1;
            goto consume;
          }
          return false;
        case rotation_90_2:
          if (*lo == '0') {
            proj.rotation = 1;
            mState = satisfied;
            lo += 1;
            goto consume;
          }
          return false;
        case rotation_180_2:
          if (*lo == '8') {
            mState = rotation_180_3;
            lo += 1;
            goto consume;
          }
          return false;
        case rotation_180_3:
          if (*lo == '0') {
            proj.rotation = 2;
            mState = satisfied;
            lo += 1;
            goto consume;
          }
          return false;
        case rotation_270_2:
          if (*lo == '7') {
            mState = rotation_270_3;
            lo += 1;
            goto consume;
          }
          return false;
        case rotation_270_3:
          if (*lo == '0') {
            proj.rotation = 3;
            mState = satisfied;
            lo += 1;
            goto consume;
          }
          return false;
        case satisfied:
          return false;
        }
      }

      return mState == satisfied;
    }

  private:
    enum State {
      real_width_start,
      real_width_continued,
      real_height_start,
      real_height_continued,
      virtual_width_start,
      virtual_width_continued,
      virtual_height_start,
      virtual_height_continued,
      rotation_start,
      rotation_90_2,
      rotation_180_2,
      rotation_180_3,
      rotation_270_2,
      rotation_270_3,
      satisfied,
    };

    State mState;

    inline bool
    isDigit(int input) {
      return input >= '0' && input <= '9';
    }
  };

  const uint32_t MAX_WIDTH = 10000;
  const uint32_t MAX_HEIGHT = 10000;

  uint32_t realWidth;
  uint32_t realHeight;
  uint32_t virtualWidth;
  uint32_t virtualHeight;
  uint32_t rotation;

  Projection()
    : realWidth(0),
      realHeight(0),
      virtualWidth(0),
      virtualHeight(0),
      rotation(0) {
  }

  void
  forceMaximumSize() {
    if (virtualWidth > realWidth) {
      virtualWidth = realWidth;
    }

    if (virtualHeight > realHeight) {
      virtualHeight = realHeight;
    }
  }

  void
  forceAspectRatio() {
    double aspect = static_cast<double>(realWidth) / static_cast<double>(realHeight);

    if (virtualHeight > (uint32_t) (virtualWidth / aspect)) {
      virtualHeight = static_cast<uint32_t>(round(virtualWidth / aspect));
    }
    else {
      virtualWidth = static_cast<uint32_t>(round(virtualHeight * aspect));
    }
  }

  bool
  valid() {
    return realWidth > 0 && realHeight > 0 &&
        virtualWidth > 0 && virtualHeight > 0 &&
        virtualWidth <= realWidth && virtualHeight <= realHeight;
  }

  friend std::ostream&
  operator<< (std::ostream& stream, const Projection& proj) {
    stream << proj.realWidth << 'x' << proj.realHeight << '@'
        << proj.virtualWidth << 'x' << proj.virtualHeight << '/' << proj.rotation;
    return stream;
  }
};

#endif
