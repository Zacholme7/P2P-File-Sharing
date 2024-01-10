#include "util.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <sys/poll.h>
#include <sys/socket.h>
#include <vector>

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void add_to_pfds(int newfd, std::vector<struct pollfd> &pfds) {
  if (pfds.size() == pfds.capacity()) {
    pfds.reserve(pfds.size() * 2);
  }
  pfds.push_back({newfd, POLLIN, 0});
}

void del_from_pfds(size_t index, std::vector<struct pollfd> &pfds) {
  std::swap(pfds[index], pfds.back());
  pfds.pop_back();
}
