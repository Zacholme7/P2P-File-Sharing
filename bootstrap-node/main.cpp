#include "bootstrapServer.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "logger.h"

Logger logger(LogLevel::Debug);

int main() {
  BootstrapServer bootstrapServer;
  std::string port = "50000";
  bootstrapServer.startServer(port);
  return 0;
}
