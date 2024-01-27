#include "bootstrapServer.h"
#include "logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Logger logger(LogLevel::Debug);

int main() {
  // construct and start the bootstrap server
  BootstrapServer bootstrapServer;
  std::string port = "50000";
  bootstrapServer.startServer(port);
  return 0;
}
