#include <fcntl.h>
#include <getopt.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>

#include <cmath>
#include <condition_variable>
#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>

#include <Minicap.hpp>

#include "util/debug.h"
#include "JpgEncoder.hpp"
#include "SimpleServer.hpp"
#include "Projection.hpp"

#define BANNER_VERSION 1
#define BANNER_SIZE 24

#define DEFAULT_SOCKET_NAME "minicap"
#define DEFAULT_DISPLAY_ID 0
#define DEFAULT_JPG_QUALITY 80

enum {
  QUIRK_DUMB            = 1,
  QUIRK_ALWAYS_UPRIGHT  = 2,
  QUIRK_TEAR            = 4,
};

static void
usage(const char* pname) {
  fprintf(stderr,
    "Usage: %s [-h] [-n <name>]\n"
    "  -d <id>:       Display ID. (%d)\n"
    "  -n <name>:     Change the name of the abtract unix domain socket. (%s)\n"
    "  -P <value>:    Display projection (<w>x<h>@<w>x<h>/{0|90|180|270}).\n"
    "  -s:            Take a screenshot and output it to stdout. Needs -P.\n"
    "  -S:            Skip frames when they cannot be consumed quickly enough.\n"
    "  -t:            Attempt to get the capture method running, then exit.\n"
    "  -i:            Get display information in JSON format. May segfault.\n"
    "  -h:            Show help.\n",
    pname, DEFAULT_DISPLAY_ID, DEFAULT_SOCKET_NAME
  );
}

class FrameWaiter: public Minicap::FrameAvailableListener {
public:
  FrameWaiter()
    : mPendingFrames(0),
      mTimeout(std::chrono::milliseconds(100)),
      mStopped(false) {
  }

  int
  waitForFrame() {
    std::unique_lock<std::mutex> lock(mMutex);

    while (!mStopped) {
      if (mCondition.wait_for(lock, mTimeout, [this]{return mPendingFrames > 0;})) {
        return mPendingFrames--;
      }
    }

    return 0;
  }

  void
  reportExtraConsumption(int count) {
    std::unique_lock<std::mutex> lock(mMutex);
    mPendingFrames -= count;
  }

  void
  onFrameAvailable() {
    std::unique_lock<std::mutex> lock(mMutex);
    mPendingFrames += 1;
    mCondition.notify_one();
  }

  void
  stop() {
    mStopped = true;
  }

  bool
  isStopped() {
    return mStopped;
  }

private:
  std::mutex mMutex;
  std::condition_variable mCondition;
  std::chrono::milliseconds mTimeout;
  int mPendingFrames;
  bool mStopped;
};

static int
pumps(int fd, unsigned char* data, size_t length) {
  do {
    // Make sure that we don't generate a SIGPIPE even if the socket doesn't
    // exist anymore. We'll still get an EPIPE which is perfect.
    int wrote = send(fd, data, length, MSG_NOSIGNAL);

    if (wrote < 0) {
      return wrote;
    }

    data += wrote;
    length -= wrote;
  }
  while (length > 0);

  return 0;
}

static int
pumpf(int fd, unsigned char* data, size_t length) {
  do {
    int wrote = write(fd, data, length);

    if (wrote < 0) {
      return wrote;
    }

    data += wrote;
    length -= wrote;
  }
  while (length > 0);

  return 0;
}

static int
putUInt32LE(unsigned char* data, int value) {
  data[0] = (value & 0x000000FF) >> 0;
  data[1] = (value & 0x0000FF00) >> 8;
  data[2] = (value & 0x00FF0000) >> 16;
  data[3] = (value & 0xFF000000) >> 24;
}

static int
try_get_framebuffer_display_info(uint32_t displayId, Minicap::DisplayInfo* info) {
  char path[64];
  sprintf(path, "/dev/graphics/fb%d", displayId);

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    MCERROR("Cannot open %s", path);
    return -1;
  }

  fb_var_screeninfo vinfo;
  if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
    close(fd);
    MCERROR("Cannot get FBIOGET_VSCREENINFO of %s", path);
    return -1;
  }

  info->width = vinfo.xres;
  info->height = vinfo.yres;
  info->orientation = vinfo.rotate;
  info->xdpi = static_cast<float>(vinfo.xres) / static_cast<float>(vinfo.width) * 25.4;
  info->ydpi = static_cast<float>(vinfo.yres) / static_cast<float>(vinfo.height) * 25.4;
  info->size = std::sqrt(
    (static_cast<float>(vinfo.width) * static_cast<float>(vinfo.width)) +
    (static_cast<float>(vinfo.height) * static_cast<float>(vinfo.height))) / 25.4;
  info->density = std::sqrt(
    (static_cast<float>(vinfo.xres) * static_cast<float>(vinfo.xres)) +
    (static_cast<float>(vinfo.yres) * static_cast<float>(vinfo.yres))) / info->size;
  info->secure = false;
  info->fps = 0;

  close(fd);

  return 0;
}

static FrameWaiter gWaiter;

static void
signal_handler(int signum) {
  switch (signum) {
  case SIGINT:
    MCINFO("Received SIGINT, stopping");
    gWaiter.stop();
    break;
  case SIGTERM:
    MCINFO("Received SIGTERM, stopping");
    gWaiter.stop();
    break;
  default:
    abort();
    break;
  }
}

int
main(int argc, char* argv[]) {
  const char* pname = argv[0];
  const char* sockname = DEFAULT_SOCKET_NAME;
  uint32_t displayId = DEFAULT_DISPLAY_ID;
  unsigned int quality = DEFAULT_JPG_QUALITY;
  bool showInfo = false;
  bool takeScreenshot = false;
  bool skipFrames = false;
  bool testOnly = false;
  Projection proj;

  int opt;
  while ((opt = getopt(argc, argv, "d:n:P:siSth")) != -1) {
    switch (opt) {
    case 'd':
      displayId = atoi(optarg);
      break;
    case 'n':
      sockname = optarg;
      break;
    case 'P': {
      Projection::Parser parser;
      if (!parser.parse(proj, optarg, optarg + strlen(optarg))) {
        std::cerr << "ERROR: invalid format for -P, need <w>x<h>@<w>x<h>/{0|90|180|270}" << std::endl;
        return EXIT_FAILURE;
      }
      break;
    }
    case 's':
      takeScreenshot = true;
      break;
    case 'i':
      showInfo = true;
      break;
    case 'S':
      skipFrames = true;
      break;
    case 't':
      testOnly = true;
      break;
    case 'h':
      usage(pname);
      return EXIT_SUCCESS;
    case '?':
    default:
      usage(pname);
      return EXIT_FAILURE;
    }
  }

  // Set up signal handler.
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  // Start Android's thread pool so that it will be able to serve our requests.
  minicap_start_thread_pool();

  if (showInfo) {
    Minicap::DisplayInfo info;

    if (minicap_try_get_display_info(displayId, &info) != 0) {
      if (try_get_framebuffer_display_info(displayId, &info) != 0) {
        MCERROR("Unable to get display info");
        return EXIT_FAILURE;
      }
    }

    int rotation;
    switch (info.orientation) {
    case Minicap::ORIENTATION_0:
      rotation = 0;
      break;
    case Minicap::ORIENTATION_90:
      rotation = 90;
      break;
    case Minicap::ORIENTATION_180:
      rotation = 180;
      break;
    case Minicap::ORIENTATION_270:
      rotation = 270;
      break;
    }

    std::cout.precision(2);
    std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);

    std::cout << "{"                                         << std::endl
              << "    \"id\": "       << displayId    << "," << std::endl
              << "    \"width\": "    << info.width   << "," << std::endl
              << "    \"height\": "   << info.height  << "," << std::endl
              << "    \"xdpi\": "     << info.xdpi    << "," << std::endl
              << "    \"ydpi\": "     << info.ydpi    << "," << std::endl
              << "    \"size\": "     << info.size    << "," << std::endl
              << "    \"density\": "  << info.density << "," << std::endl
              << "    \"fps\": "      << info.fps     << "," << std::endl
              << "    \"secure\": "   << (info.secure ? "true" : "false") << "," << std::endl
              << "    \"rotation\": " << rotation            << std::endl
              << "}"                                         << std::endl;

    return EXIT_SUCCESS;
  }

  proj.forceMaximumSize();
  proj.forceAspectRatio();

  if (!proj.valid()) {
    std::cerr << "ERROR: missing or invalid -P" << std::endl;
    return EXIT_FAILURE;
  }

  std::cerr << "PID: " << getpid() << std::endl;
  std::cerr << "INFO: Using projection " << proj << std::endl;

  // Disable STDOUT buffering.
  setbuf(stdout, NULL);

  // Set real display size.
  Minicap::DisplayInfo realInfo;
  realInfo.width = proj.realWidth;
  realInfo.height = proj.realHeight;

  // Figure out desired display size.
  Minicap::DisplayInfo desiredInfo;
  desiredInfo.width = proj.virtualWidth;
  desiredInfo.height = proj.virtualHeight;
  desiredInfo.orientation = proj.rotation;

  // Leave a 4-byte padding to the encoder so that we can inject the size
  // to the same buffer.
  JpgEncoder encoder(4, 0);
  Minicap::Frame frame;
  bool haveFrame = false;

  // Server config.
  SimpleServer server;

  // Set up minicap.
  Minicap* minicap = minicap_create(displayId);
  if (minicap == NULL) {
    return EXIT_FAILURE;
  }

  // Figure out the quirks the current capture method has.
  unsigned char quirks = 0;
  switch (minicap->getCaptureMethod()) {
  case Minicap::METHOD_FRAMEBUFFER:
    quirks |= QUIRK_DUMB | QUIRK_TEAR;
    break;
  case Minicap::METHOD_SCREENSHOT:
    quirks |= QUIRK_DUMB;
    break;
  case Minicap::METHOD_VIRTUAL_DISPLAY:
    quirks |= QUIRK_ALWAYS_UPRIGHT;
    break;
  }

  if (minicap->setRealInfo(realInfo) != 0) {
    MCERROR("Minicap did not accept real display info");
    goto disaster;
  }

  if (minicap->setDesiredInfo(desiredInfo) != 0) {
    MCERROR("Minicap did not accept desired display info");
    goto disaster;
  }

  minicap->setFrameAvailableListener(&gWaiter);

  if (minicap->applyConfigChanges() != 0) {
    MCERROR("Unable to start minicap with current config");
    goto disaster;
  }

  if (!encoder.reserveData(realInfo.width, realInfo.height)) {
    MCERROR("Unable to reserve data for JPG encoder");
    goto disaster;
  }

  if (takeScreenshot) {
    if (!gWaiter.waitForFrame()) {
      MCERROR("Unable to wait for frame");
      goto disaster;
    }

    int err;
    if ((err = minicap->consumePendingFrame(&frame)) != 0) {
      MCERROR("Unable to consume pending frame");
      goto disaster;
    }

    if (!encoder.encode(&frame, quality)) {
      MCERROR("Unable to encode frame");
      goto disaster;
    }

    if (pumpf(STDOUT_FILENO, encoder.getEncodedData(), encoder.getEncodedSize()) < 0) {
      MCERROR("Unable to output encoded frame data");
      goto disaster;
    }

    return EXIT_SUCCESS;
  }

  if (testOnly) {
    if (gWaiter.waitForFrame() <= 0) {
      MCERROR("Did not receive any frames");
      std::cout << "FAIL" << std::endl;
      return EXIT_FAILURE;
    }

    minicap_free(minicap);
    std::cout << "OK" << std::endl;
    return EXIT_SUCCESS;
  }

  if (!server.start(sockname)) {
    MCERROR("Unable to start server on namespace '%s'", sockname);
    goto disaster;
  }

  // Prepare banner for clients.
  unsigned char banner[BANNER_SIZE];
  banner[0] = (unsigned char) BANNER_VERSION;
  banner[1] = (unsigned char) BANNER_SIZE;
  putUInt32LE(banner + 2, getpid());
  putUInt32LE(banner + 6,  realInfo.width);
  putUInt32LE(banner + 10,  realInfo.height);
  putUInt32LE(banner + 14, desiredInfo.width);
  putUInt32LE(banner + 18, desiredInfo.height);
  banner[22] = (unsigned char) desiredInfo.orientation;
  banner[23] = quirks;

  int fd;
  while (!gWaiter.isStopped() && (fd = server.accept()) > 0) {
    MCINFO("New client connection");

    if (pumps(fd, banner, BANNER_SIZE) < 0) {
      close(fd);
      continue;
    }

    int pending, err;
    while (!gWaiter.isStopped() && (pending = gWaiter.waitForFrame()) > 0) {
      if (skipFrames && pending > 1) {
        // Skip frames if we have too many. Not particularly thread safe,
        // but this loop should be the only consumer anyway (i.e. nothing
        // else decreases the frame count).
        gWaiter.reportExtraConsumption(pending - 1);

        while (--pending >= 1) {
          if ((err = minicap->consumePendingFrame(&frame)) != 0) {
            if (err == -EINTR) {
              MCINFO("Frame consumption interrupted by EINTR");
              goto close;
            }
            else {
              MCERROR("Unable to skip pending frame");
              goto disaster;
            }
          }

          minicap->releaseConsumedFrame(&frame);
        }
      }

      if ((err = minicap->consumePendingFrame(&frame)) != 0) {
        if (err == -EINTR) {
          MCINFO("Frame consumption interrupted by EINTR");
          goto close;
        }
        else {
          MCERROR("Unable to consume pending frame");
          goto disaster;
        }
      }

      haveFrame = true;

      // Encode the frame.
      if (!encoder.encode(&frame, quality)) {
        MCERROR("Unable to encode frame");
        goto disaster;
      }

      // Push it out synchronously because it's fast and we don't care
      // about other clients.
      unsigned char* data = encoder.getEncodedData() - 4;
      size_t size = encoder.getEncodedSize();

      putUInt32LE(data, size);

      if (pumps(fd, data, size + 4) < 0) {
        break;
      }

      // This will call onFrameAvailable() on older devices, so we have
      // to do it here or the loop will stop.
      minicap->releaseConsumedFrame(&frame);
      haveFrame = false;
    }

close:
    MCINFO("Closing client connection");
    close(fd);

    // Have we consumed one frame but are still holding it?
    if (haveFrame) {
      minicap->releaseConsumedFrame(&frame);
    }
  }

  minicap_free(minicap);

  return EXIT_SUCCESS;

disaster:
  if (haveFrame) {
    minicap->releaseConsumedFrame(&frame);
  }

  minicap_free(minicap);

  return EXIT_FAILURE;
}
