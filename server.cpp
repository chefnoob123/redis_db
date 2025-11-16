#include <arpa/inet.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <errno.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static void msg(const char *msg) { fprintf(stderr, "%s\n", msg); }

static void die(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] %s\n", err, msg);
  abort();
}
// we implement read_full instead of read because read can return less than the
// requested amount of bytes uder normal conditions

static int32_t read_full(int fd, char *buf, size_t n) {
  while (n > 0) {
    ssize_t rv = read(fd, buf, n);
    if (rv <= 0) {
      return -1; // error
    }
    // this is a single attempt to write which could lead to short reads
    assert((ssize_t)rv <= n); // sanity check so that read never return more
                              // bytes than it is supposed to read
    n -= (size_t)rv; // subtract the number of bytes we still need to read from
                     // those that were read
    buf += rv;       // moves the buffer pointer {prevents overwriting}
  }
  return 0;
}

static int32_t write_all(int fd, char *buf,
                         size_t n) { // n: number of bytes to actually write
  while (n > 0) {
    ssize_t rv = write(fd, buf, n);
    if (rv <= 0) {
      return -1; // error
    }
    // this is a single attempt to write which could lead to short writes
    assert((ssize_t)rv <= n);
    n -= (ssize_t)rv;
    buf += rv;
  }
  return 0;
}

const size_t k_max_msg = 4096;

static int32_t one_request(int connfd) {
  // 4 bytes header
  char rbuf[4 + k_max_msg];
  errno = 0;
  int32_t err = read_full(connfd, rbuf, 4); // first read the header
  if (err) {
    msg(errno == 0 ? "EOF" : "read() error");
    return err;
  }
  uint32_t len = 0;
  memcpy(&len, rbuf, 4);
  if (len > k_max_msg) {
    msg("too long");
    return -1;
  }
  // request body
  err = read_full(connfd, &rbuf[4], len);
  if (err) {
    msg("read() error");
    return err;
  }
  // do something
  printf("client says: %.*s\n", len, &rbuf[4]);
  const char reply[] = "world";
  char wbuf[4 + sizeof(reply)];
  len = (uint32_t)strlen(reply);
  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], reply, len);
  return write_all(connfd, wbuf, 4 + len);
}

static void do_something(int connfd) {
  char rbuf[64] = {};
  ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
  if (n < 0) {
    msg("read() error");
    return;
  }
  fprintf(stderr, "client says: %s\n", rbuf);

  char wbuf[] = "world";
  write(connfd, wbuf, strlen(wbuf));
}

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    die("socket()");
  }

  // this is needed for most server applications
  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // bind
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = ntohs(1234);
  addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
  int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("bind()");
  }

  // listen
  rv = listen(fd, SOMAXCONN);
  if (rv) {
    die("listen()");
  }

  while (true) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd < 0) {
      continue; // error
    }

    do_something(connfd);
    close(connfd);
  }

  return 0;
}
