// helper program for server-side child process.

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
#include <ctype.h>

#define EXIT_HDERR( condition ) \
    if( condition ) { \
      writeSerial(cfd, "HDERR\n", strlen("HDERR\n")); \
      close(cfd); \
      _exit(EXIT_FAILURE); \
    } 
    

#define EXIT_ON_FAIL( num, arg, error_msg, cfd ) \
    if( num  == (arg) ) { \
      close(cfd); \
      fprintf( stderr, "%s\n", error_msg ); \
      _exit( EXIT_FAILURE ); \
    } 

void writeSerial(int cfd, char* str, int length) {
  ssize_t numWritten = 0;
  while (numWritten < length) {
    ssize_t sent = write(cfd, str + numWritten, length);
    EXIT_ON_FAIL(-1, sent, "Error writing serial.", cfd);
    numWritten += sent;
  }
}

void readHeaderChunk(int cfd, char* chunk, int length, int alnum, int noslash) {
  ssize_t bytes_read = 0;
  char buffer[2] = {0};
  memset(chunk, 0, length);

  while (bytes_read < length) {
    ssize_t numRead = read(cfd, buffer, 1);
    EXIT_HDERR(numRead <= 0);
    
    if (buffer[0] == '\n') {
      EXIT_HDERR(0 == bytes_read);
      return;
    };

    *(chunk + bytes_read) = buffer[0];
    EXIT_HDERR((alnum && !isalnum(buffer[0])) || (noslash && buffer[0] == '/'));

    bytes_read += numRead;
  }
  EXIT_HDERR(1);
}

void readFileAndSave(int cfd, char *username, char *serial, int filesize, char *filename) {
  char newFilename[121] = {0};
  char buffer[100] = {0};
  snprintf(newFilename, sizeof(newFilename) - 1, "%s-%s-%s", username, serial, filename);

  FILE *newFile = fopen(newFilename, "wb");
  EXIT_ON_FAIL(NULL, newFile, "Couldn't open file.", cfd)

  ssize_t bytes_checked = 0;
  while (bytes_checked < filesize) {
    memset(buffer, 0, 20);
    int bufferSize = (filesize - bytes_checked < 20) ? filesize - bytes_checked : 20;
    ssize_t numRead = read(cfd, buffer, bufferSize);

    if (numRead <= 0) {
      fclose(newFile);
      remove(newFilename);
    } else {
      fwrite(buffer, 1, numRead, newFile);
      bytes_checked += numRead;
    }
    EXIT_ON_FAIL(-1, numRead, "Error with read", cfd);
    EXIT_ON_FAIL(0, numRead, "Fewer bytes than promised", cfd);
  }
  fclose(newFile);

}

int main(int argc, char *argv[]) {
  char username[9];
  char filename[101];
  char filesizeStr[11];
  long long filesize;
  char serial[11] = {0};

  // cfd, serial
  int cfd = atoi(argv[1]);

  readHeaderChunk(cfd, username, 9, 1, 0);
  readHeaderChunk(cfd, filename, 101, 0, 1);
  readHeaderChunk(cfd, filesizeStr, 11, 0, 0);

  // convert filesize to number
  char *debug;
  filesize = strtoll(filesizeStr, &debug, 10);

  // read file contents
  readFileAndSave(cfd, username, argv[2], filesize, filename);

  memcpy(serial, argv[2], 10);
  serial[strlen(argv[2])] = '\n';
  writeSerial(cfd, serial, strlen(serial));

  close(cfd);
  exit(0);
}
