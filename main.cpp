#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>

void die() int main() {

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  int val = 1;
  /*
   SO_REUSEADDR so that a new sever program can bind to the same IP and PORT
   number incase the previous connection was closed
   */
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = htonl(0);

  int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));

  if (rv) {
    die("bind()");
  }
}
