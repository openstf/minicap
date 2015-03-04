#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include <minicap.hpp>

#include "util/debug.h"

static void
usage(const char* pname)
{
  fprintf(stderr,
    "Usage: %s [-h]\n"
    "  -h:          Show help.\n",
    pname
  );
}

int main(int argc, char* argv[]) {
  const char* pname = argv[0];

  int opt;
  while ((opt = getopt(argc, argv, "h")) != -1) {
    switch (opt) {
      case '?':
        usage(pname);
        return EXIT_FAILURE;
      case 'h':
        usage(pname);
        return EXIT_SUCCESS;
    }
  }

  // Disable STDOUT buffering.
  setbuf(stdout, NULL);

  // Start Android's thread pool so that it will be able to serve our requests.
  minicap_start_thread_pool();

  int32_t display_id = 0;
  uint32_t real_width = 2560, real_height = 1600;

  minicap* mc = minicap_create(display_id);

  // This should never happen. Check anyways.
  if (mc == NULL) {
    MCERROR("Unable to create minicap");
    return EXIT_FAILURE;
  }

  mc->set_real_size(real_width, real_height);
  mc->set_desired_projection(real_width, real_height, 0);

  if (mc->begin_updates() != 0) {
    MCERROR("Unable to prepare minicap for updates");
  }
  else {
    do {
      if (mc->update() != 0) {
        MCERROR("Unable to get new frame");
        break;
      }

      write(STDOUT_FILENO, mc->get_pixels(), mc->get_size());
      //for (int y = 0; y < mc->get_height(); ++y) {
      //  write(STDOUT_FILENO, mc->get_pixels() + (y * mc->get_stride() * mc->get_bpp()), mc->get_width() * mc->get_bpp());
      //}
    }
    while (1);
  }

  minicap_free(mc);

  return EXIT_SUCCESS;
}
