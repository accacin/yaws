#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 10
#define MAXLINE 4096

int main(int argc, char *argv[]) {
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  int sockfd, new_fd, rv, n;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  uint8_t buff[MAXLINE + 1];
  uint8_t recvline[MAXLINE + 1];

  if (argc != 2) {
    fprintf(stderr, "no port specified\n");
    return 1;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo);

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
  printf("Server: listening on port %s\n", argv[1]);
  printf("Waiting for connections...\n");

  for (;;) {
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    memset(recvline, 0, MAXLINE);

    while ((n = read(new_fd, recvline, MAXLINE - 1)) != -1) {
      fprintf(stdout, "\n%s", recvline);

      if (recvline[n - 1] == '\n') {
        break;
      }

      memset(recvline, 0, MAXLINE);
    }

    if (n < 0) {
      perror("read error");
      exit(1);
    }

    snprintf((char *)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\nHello");
    write(new_fd, (char *)buff, strlen((char *)buff));
    close(new_fd);
  }
}
