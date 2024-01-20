#include "bootstrapServer.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  BootstrapServer bootstrapServer;
  std::string port = "50000";
  bootstrapServer.startServer(port);
  return 0;
}
