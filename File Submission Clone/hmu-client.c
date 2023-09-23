// Client program.

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

#define _FILE_OFFSET_BITS 64

#define EXIT_ON_FAIL( condition, error_msg, cfd ) \
    if( condition ) { \
      fprintf( stderr, "%s\n", error_msg ); \
      close(cfd); \
      exit( EXIT_FAILURE ); \
    } 

#define EXIT_ON_FAIL_NO_CFD( condition, error_msg) \
    if( condition ) { \
      fprintf( stderr, "%s\n", error_msg ); \
      exit( EXIT_FAILURE ); \
    } 

void cpyAppendNL(char *str, char *dest) {
  int len = strlen(str);
  memcpy(dest, str, len);
  dest[len] = '\n';
  dest[len + 1] = '\0';
}

void writeToServer(int socket, char *str) {
  ssize_t len = strlen(str);
  ssize_t bytes_sent = 0;
  while (bytes_sent < len) {
    ssize_t sent = write(socket, str + bytes_sent, len - bytes_sent);
    EXIT_ON_FAIL(sent == -1, "socket write", socket);
    bytes_sent += sent;
  }
}

void readFromServer(int socket, char *str, int length) {
  ssize_t bytes_read = 0;
  char buffer[2] = {0};
  while (bytes_read < length) {
    ssize_t numRead = read(socket, buffer, 1);
    EXIT_ON_FAIL(numRead <= 0, "socket read", socket);
    *(str + bytes_read) = buffer[0];
    if (buffer[0] == '\n') break;
    bytes_read += numRead;
  }
}

// cmdline reminder: IP-address, portnum, username, filename
int main(int argc, char *argv[]) {
  FILE *file;
  char username[9] = {0};
  char filename[101] = {0};
  char filesize[11] = {0};
  long long end;
  long long port;
  char *debug;
  char endStr[11];
  int cfd;
  struct sockaddr_in a;
  int numRead;

  EXIT_ON_FAIL_NO_CFD((file = fopen(argv[4], "rb")) == NULL, "open file")

  port = strtoll(argv[2], &debug, 10);
  EXIT_ON_FAIL_NO_CFD(*debug != '\0', "Invalid port num.")
  
  cpyAppendNL(argv[3], username);
  cpyAppendNL(argv[4], filename);

  // get filesize (bytes)
  fseeko(file, 0L, SEEK_END);
  end = (long long)ftello(file);
  snprintf(endStr, 11, "%lld", end);
  cpyAppendNL(endStr, filesize);

  // init addr struct
  memset(&a, 0, sizeof(struct sockaddr_in));
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  EXIT_ON_FAIL_NO_CFD(!inet_pton(AF_INET, argv[1], &a.sin_addr), "invalid IPv4 address")

  // make and connect socket
  cfd = socket(AF_INET, SOCK_STREAM, 0);
  EXIT_ON_FAIL(-1 == connect(cfd, (struct sockaddr *)&a, sizeof(struct sockaddr_in)), "connect", cfd);

  // write username, filename, file size
  writeToServer(cfd, username);
  writeToServer(cfd, filename);
  writeToServer(cfd, filesize);

  // write content
  char content_buffer[50] = {0};
  fseeko(file, 0L, SEEK_SET);
  while ((numRead = fread(content_buffer, 1, sizeof(content_buffer), file))) {
    writeToServer(cfd, content_buffer);
    memset(content_buffer, '\0', 50);
  }

  // display serial number
  char serial_num[12] = {0};
  readFromServer(cfd, serial_num, 12);
  serial_num[11] = '\0';
  char *newline;
  EXIT_ON_FAIL((newline = strchr(serial_num, '\n')) == NULL, "invalid serial num", cfd);

  *(newline + 1) = '\0';
  for (char *letter = serial_num; letter < newline; letter += 1) {
    if (*letter < '0' || *letter > '9') {
      fprintf(stderr, "Serial number error: %s", serial_num);
      close(cfd);
      exit(1);
    }
  }
  printf("%s", serial_num);
  
  close(cfd);
  fclose(file);
  return 0;
}
