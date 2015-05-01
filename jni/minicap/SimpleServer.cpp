#include "SimpleServer.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

SimpleServer::SimpleServer(): mFd(0) {
}

SimpleServer::~SimpleServer() {
  if (mFd > 0) {
    ::close(mFd);
  }
}

int
SimpleServer::start(const char* sockname) {
  int sfd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (sfd < 0) {
    return sfd;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(&addr.sun_path[1], sockname, strlen(sockname));

  if (::bind(sfd, (struct sockaddr*) &addr,
      sizeof(sa_family_t) + strlen(sockname) + 1) < 0) {
    ::close(sfd);
    return -1;
  }

  ::listen(sfd, 1);

  mFd = sfd;

  return mFd;
}

int
SimpleServer::accept() {
  struct sockaddr_un addr;
  socklen_t addr_len = sizeof(addr);
  return ::accept(mFd, (struct sockaddr *) &addr, &addr_len);
}
