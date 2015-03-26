#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

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

#define DEFAULT_SOCKET_NAME "minicap"
#define DEFAULT_JPG_QUALITY 80

static void
usage(const char* pname)
{
  fprintf(stderr,
    "Usage: %s [-h] [-n <name>]\n"
    "  -n <name>:   Change the name of of the abtract unix domain socket. (%s)\n"
    "  -h:          Show help.\n",
    pname, DEFAULT_SOCKET_NAME
  );
}

class FrameWaiter: public Minicap::FrameAvailableListener {
public:
  FrameWaiter()
    : mPendingFrames(0),
      mTimeout(std::chrono::milliseconds(100)),
      mStopped(false) {
  }

  bool
  waitForFrame() {
    std::unique_lock<std::mutex> lock(mMutex);

    while (!mStopped) {
      if (mCondition.wait_for(lock, mTimeout, [this]{return mPendingFrames > 0;})) {
        mPendingFrames -= 1;
        return !mStopped;
      }
    }

    return false;
  }

  void
  onFrameAvailable() {
    std::unique_lock<std::mutex> lock(mMutex);
    mPendingFrames += 1;
    mCondition.notify_one();
  }

  void
  cancel() {
    mStopped = true;
  }

private:
  std::mutex mMutex;
  std::condition_variable mCondition;
  std::chrono::milliseconds mTimeout;
  int mPendingFrames;
  bool mStopped;
};

int
pump(int fd, unsigned char* data, size_t length) {
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

int
main(int argc, char* argv[]) {
  const char* pname = argv[0];
  const char* sockname = DEFAULT_SOCKET_NAME;
  unsigned int quality = DEFAULT_JPG_QUALITY;

  // Disable STDOUT buffering.
  setbuf(stdout, NULL);

  // Start Android's thread pool so that it will be able to serve our requests.
  minicap_start_thread_pool();

  // Set real display size.
  Minicap::DisplayInfo realInfo;
  realInfo.width = 1080;
  realInfo.height = 1920;

  // Figure out desired display size.
  Minicap::DisplayInfo desiredInfo;
  desiredInfo.width = 720;
  desiredInfo.height = 1280;
  desiredInfo.orientation = 0;

  // Leave a 4-byte padding to the encoder so that we can inject the size
  // to the same buffer.
  JpgEncoder encoder(4, 0);
  FrameWaiter waiter;
  Minicap::Frame frame;
  bool haveFrame = false;

  // Server config.
  SimpleServer server;

  // Set up minicap.
  Minicap* minicap = minicap_create(0);
  if (minicap == NULL) {
    return EXIT_FAILURE;
  }

  if (!minicap->setRealInfo(realInfo)) {
    MCERROR("Minicap did not accept real display info");
    goto disaster;
  }

  if (!minicap->setDesiredInfo(desiredInfo)) {
    MCERROR("Minicap did not accept desired display info");
    goto disaster;
  }

  minicap->setFrameAvailableListener(&waiter);

  if (!minicap->applyConfigChanges()) {
    MCERROR("Unable to start minicap with current config");
    goto disaster;
  }

  if (!encoder.reserveData(realInfo.width, realInfo.height)) {
    MCERROR("Unable to reserve data for JPG encoder");
    goto disaster;
  }

  if (!server.start(sockname)) {
    MCERROR("Unable to start server on namespace '%s'", sockname);
    goto disaster;
  }

  int fd;
  while ((fd = server.accept()) > 0) {
    MCINFO("New client connection");

    while (waiter.waitForFrame()) {
      if (haveFrame) {
        minicap->releaseConsumedFrame(&frame);
        haveFrame = false;
      }

      if (!minicap->consumePendingFrame(&frame)) {
        MCERROR("Unable to consume pending frame");
        goto disaster;
      }

      haveFrame = true;

      if (!encoder.encode(&frame, quality)) {
        MCERROR("Unable to encode frame");
        goto disaster;
      }

      unsigned char* data = encoder.getEncodedData() - 4;
      size_t size = encoder.getEncodedSize();

      data[0] = (size & 0x000000FF) >> 0;
      data[1] = (size & 0x0000FF00) >> 8;
      data[2] = (size & 0x00FF0000) >> 16;
      data[3] = (size & 0xFF000000) >> 24;

      if (pump(fd, data, size + 4) < 0) {
        break;
      }
    }

    MCINFO("Closing client connection");

    close(fd);
  }

  if (haveFrame) {
    minicap->releaseConsumedFrame(&frame);
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
