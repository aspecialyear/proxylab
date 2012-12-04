#include "proxylhelpers.h"

/**
 * Pgethostbyname - Wrapper of thread safe get host by name and IP.
 * Will call Pgethostbyaddr if the addr is IP address
 */
struct hostent *Pgethostbyname(const char *addr)
{
  static sem_t mutex;
  struct hostent *hostp;
  struct in_addr addrp;
  sem_init(&mutex, 0, 1);
  P(&mutex);
  if (inet_aton(addr, &addrp) != 0)
    /* The host name is IP address */
    hostp = gethostbyaddr((const char *)&addrp, sizeof(addrp), AF_INET);
  else
    hostp = gethostbyname(addr);
  return hostp;
}

/**
 * Modified to use Pgethostbyname
 */
int Popen_clientfd(char *hostname, int port)
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = Pgethostbyname(hostname)) == NULL)
	return -2; /* check h_errno for cause of error */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], 
	  (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
	return -1;
    return clientfd;
}

int POpen_clientfd(char *hostname, int port) 
{
    int rc;

    if ((rc = Popen_clientfd(hostname, port)) < 0) {
	if (rc == -1)
	    unix_error("Open_clientfd Unix error");
	else       
	    dns_error("Open_clientfd DNS error");
    }
    return rc;
}

void printbychar(char *in)
{
  int i;
  if (!in)
   {
     return;
   }
  printf("Print by char's digital value...\n");
  for (i = 1; (in[i - 1] != '\0'); i++)
  {
    printf("%d ", in[i-1]);
  }
  printf("\n");
}

void myClose(int fd)
{
      int rc;

      if ((rc = close(fd)) < 0)
    server_error("Close error");
}

void server_error(char *msg) /* error printing without exiting server */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}

/* Output error message to the client.
 */

void clienterror(int fd, char *cause, char *errnum,
    char *shortmsg, char *longmsg)
{
   char buf[MAXLINE], body[MAXBUF];

   /* Build the HTTP response body */
   sprintf(body, "<html><title>Proxy Error</title>");
   sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
   sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
   sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
   sprintf(body, "%s<hr><em>The Proxy server</em>\r\n", body);

   /* Print the HTTP response */
   sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
   Rio_writen(fd, buf, strlen(buf));
   sprintf(buf, "Content-type: text/html\r\n");
   Rio_writen(fd, buf, strlen(buf));
   sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
   Rio_writen(fd, buf, strlen(buf));
   Rio_writen(fd, body, strlen(body));
}
