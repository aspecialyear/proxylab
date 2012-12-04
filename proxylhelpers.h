#include "csapp.h"
#include "errno.h"

#define MR_wrerrcheck(len) \
  if (len == -1){\
    if (errno == ECONNRESET || errno == EPIPE){ \
      dbg_printf("!!!%d:%s, returning...\n", errno, strerror(errno));\
      return;\
    }else unix_error("Read error other than ECONNRESET or EPIPE");\
  }

/* Macro wrapper for rio_write and rio_read with proper error handling.*/
#define MRio_writen(outfd, buf, buflen) \
  if (rio_writen(outfd, buf, buflen) == -1){ \
    if (errno == ECONNRESET || errno == EPIPE){ \
      dbg_printf("!!!Wrote to broken pipe on fd:%d, returing...\n", outfd);\
      return;\
    } else { \
      printf("Write Error: when writing to %d with %s", outfd, (char *)buf);\
      unix_error("Write error other than EPIPE");}\
  }

#define MRio_readnb(rp, buf, n, bytesread) \
    bytesread = rio_readnb(rp, buf, n);\
    MR_wrerrcheck(bytesread);

#define MRio_readlineb(rp, buf, n) \
    MR_wrerrcheck(rio_readlineb(rp, buf, n));

struct hostent *Pgethostbyname(const char *addr);
int Popen_clientfd(char *hostname, int port);
void printbychar(char *in);
void myClose(int fd);
void server_error(char *msg); /* only log upon errors.*/

void clienterror(int fd, char *cause, char *errorCode,
    char *msg, char *longmsg);
