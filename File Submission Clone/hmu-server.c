// Server program.

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// cmdline reminder: portnum, helper
int main(int argc, char *argv[]) {
  int sfd;
  int cfd;
  pid_t child;
  struct sockaddr_in a;

  signal(SIGCHLD, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);

  int port = atoi(argv[1]);

  sfd = socket(AF_INET, SOCK_STREAM, 0);

  memset(&a, 0, sizeof(struct sockaddr_in));
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  if (-1 == bind(sfd, (struct sockaddr *)&a, sizeof(struct sockaddr_in))) {
    perror("error binding");
    close(sfd);
    exit(1);
  }

  if (-1 == listen(sfd, 3)) {
    perror("listen");
    close(sfd);
    exit(1);
  }

  long long serial = 0;
  for (;;) {
    struct sockaddr ca;
    socklen_t sinlen;
    cfd = accept(sfd, &ca, &sinlen);

    if (cfd < 0) {
      perror("Error with client connection.");
      continue;
    }

    if ((child = fork()) == -1) {
      perror("fork");
      close(sfd);
      close(cfd);
      exit(1);
    } 
    else if (child == 0) {
      close(sfd);
      char serialStr[11] = {0};
      char cfdStr[8] = {0};
      snprintf(serialStr, 11, "%lld", serial);
      snprintf(cfdStr, 8, "%d", cfd);
      if (execl(argv[2], argv[2], cfdStr, serialStr, (char *)NULL) == -1) {
        perror("exec failed.");
        close(sfd);
        close(cfd);
        _exit(1);
      }
    } else {
      close(cfd);
    }
    serial++;
  }

  close(sfd);
  return 0;
}
