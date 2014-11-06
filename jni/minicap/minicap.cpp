#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>

#include <turbojpeg.h>
#include <minicap.hpp>

#define VERSION 1
#define DEFAULT_SOCKET_NAME "minicap"
#define DEFAULT_MAX_EVENTS 64

#define UNLIKELY(x)     __builtin_expect((x),0)
#define D(...)          fprintf(stderr, __VA_ARGS__)
//#define D(...)

static void
usage(const char* pname)
{
  fprintf(stderr,
    "Usage: %s [-h] [-n <name>]\n"
    "  -n <name>: Change the name of of the abtract unix domain socket. (%s)\n"
    "  -h:        Show help.\n",
    pname, DEFAULT_SOCKET_NAME
  );
}

static int
start_abstract_server(char const* sockname)
{
  int sfd;
  struct sockaddr_un addr;

  sfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sfd < 0)
  {
    perror("creating socket");
    return sfd;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(&addr.sun_path[1], sockname, strlen(sockname));

  if (bind(sfd, (struct sockaddr*) &addr,
    sizeof(sa_family_t) + strlen(sockname) + 1) < 0)
  {
    perror("binding socket");
    close(sfd);
    return -1;
  }

  return sfd;
}

int
main(int argc, char* argv[])
{
  const char* pname = argv[0];
  char const* sockname = DEFAULT_SOCKET_NAME;
  int max_events = DEFAULT_MAX_EVENTS;
  int opt;
  int sfd, efd;
  struct sockaddr_un client_addr;
  struct minicap* mchandle = minicap_create(0);

  while ((opt = getopt(argc, argv, "n:h")) != -1) {
    switch (opt) {
      case 'n':
        sockname = optarg;
        break;
      case '?':
        usage(pname);
        return EXIT_FAILURE;
      case 'h':
        usage(pname);
        return EXIT_SUCCESS;
    }
  }

  // Start the main server
  sfd = start_abstract_server(sockname);
  if (sfd < 0)
  {
    fprintf(stderr, "Unable to start server on %s\n", sockname);
    return EXIT_FAILURE;
  }

  if (listen(sfd, SOMAXCONN) < 0)
  {
    perror("listen");
    return EXIT_FAILURE;
  }

  for (;;)
  {
    int len = sizeof(client_addr);
    int client_fd = accept(sfd, (struct sockaddr *) &client_addr, &len);

    if (client_fd < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    D("Connection established\n");

    clock_t begin, end;
    double time_spent;

    tjhandle handle;
    int subsampling = TJSAMP_420;
    int quality = 80;
    int format;
    unsigned int max_width, max_height;
    unsigned int max_q;
    unsigned long max_size;
    unsigned char* data;
    unsigned int req_width, req_height;
    unsigned int req_q;
    unsigned long size;
    int ok;

    minicap_update(mchandle, 0, 0);

    max_width = minicap_get_width(mchandle) / 2;
    max_height = minicap_get_height(mchandle) / 2;
    max_q = 100;

    switch (minicap_get_format(mchandle))
    {
      case MINICAP_FORMAT_RGBA_8888:
        format = TJPF_RGBA;
        break;
      case MINICAP_FORMAT_RGBX_8888:
        format = TJPF_RGBX;
        break;
      case MINICAP_FORMAT_RGB_888:
        format = TJPF_RGB;
        break;
      case MINICAP_FORMAT_BGRA_8888:
        format = TJPF_BGRA;
        break;
      default:
        fprintf(stderr, "Unsupported pixel format (%d)\n", minicap_get_format(mchandle));
        return EXIT_FAILURE;
    }

    max_size = tjBufSize(
      max_width,
      max_height,
      subsampling
    );

    data = tjAlloc(max_size);

    if (!data)
    {
      fprintf(stderr, "Unable to allocate conversion\n");
      return EXIT_FAILURE;
    }

    handle = tjInitCompress();

    char header_buffer[256] = {0};
    int header_len;
    char chunk_buffer[32] = {0};
    int chunk_len;
    char boundary[] = "gc0p4Jq0M2Yt08jU534c0p";

    header_len = snprintf(header_buffer, sizeof(header_buffer),
      "HTTP/1.1 200 OK\r\n"
      "Transfer-Encoding: chunked\r\n"
      "Content-Type: multipart/x-mixed-replace;boundary=%s\r\n"
      "Cache-Control: no-store,max-age=-1\r\n"
      "Connection: keep-alive\r\n"
      "\r\n",
      boundary
    );

    write(client_fd, header_buffer, header_len);

    for (;;)
    {
      req_width = 0;
      req_height = 0;
      req_q = 80;

      begin = clock();

      minicap_update(mchandle, req_width, req_height);

      ok = tjCompress2(
        handle,
        (unsigned char*) minicap_get_pixels(mchandle),
        minicap_get_width(mchandle),
        minicap_get_stride(mchandle) * minicap_get_bpp(mchandle),
        minicap_get_height(mchandle),
        format,
        &data,
        &size,
        subsampling,
        req_q,
        TJFLAG_FASTDCT | TJFLAG_NOREALLOC
      );

      if (ok != 0)
      {
        fprintf(stderr, "Conversion failed\n");
        exit(1);
      }

      end = clock();
      time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

      header_len = snprintf(header_buffer, sizeof(header_buffer),
        "--%s\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %ld\r\n"
        "\r\n",
        boundary, size
      );

      chunk_len = snprintf(chunk_buffer, sizeof(chunk_buffer),
        "%lx\r\n",
        header_len + size
      );

      write(client_fd, chunk_buffer, chunk_len);
      write(client_fd, header_buffer, header_len);
      write(client_fd, data, size);
      write(client_fd, "\r\n", 2);

      D("Took %f seconds\n", time_spent);

      usleep(100000);
    }

    D("Connection closed\n");
    close(client_fd);
    tjFree(data);
    tjDestroy(handle);
  }

  close(sfd);

  return EXIT_SUCCESS;
}
