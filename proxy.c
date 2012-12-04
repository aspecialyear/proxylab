#include "proxylhelpers.h"
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAX_HEADER_SIZE 8192

#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/* The struct to hold host name, port number and path parsed from
 * first line of header.
 */
struct parsed_request{
  char uri[MAXLINE];
  char method[MAXLINE];
  char version[MAXLINE];
  char hostName[MAXLINE];
  char path[MAXLINE];
  int port;
};

static const char *USER_AGENT = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *ACCEPT_HDR = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *ACCEPT_ENCODING = "Accept-Encoding: gzip, deflate\r\n";
static const char *HTTP_VERSION = "HTTP/1.0";

void clienterror(int fd, char *cause, char *errorCode,
		char *msg, char *longmsg);
void processRequest(int connfd);
void write_req_hrds(int outfd, struct parsed_request *rmtaddr);

/**
 * Private Helper Functions
 */
static int parse_first_line(char *first_line, struct parsed_request *rmtaddr);


int testserve(int argc, char **argv)
{
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);
  listenfd = Open_listenfd(port);
  while(1)
  {
    printf("Listening on port:%d...\n", port);
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
      (unsigned int *) (&clientlen));
    (void)processRequest(connfd);
    Close(connfd);
  }
  Close(listenfd);
  return 0;
}

int testrequestparse()
{
  struct parsed_request rmtaddr;
    char *uri = "https://www.google.com:8080/cn/index::.html";
    //uri = "ftps://www.google.com/cn/index::.html";
    uri = "GET http://74.125.131.105/cn/index.html HTTP/1.1";
    parse_first_line(uri, &rmtaddr);
    dbg_printf("%s:%s:%d\n", rmtaddr.hostName, rmtaddr.path, rmtaddr.port);
    Pgethostbyname(rmtaddr.hostName);
    write_req_hrds(STDOUT_FILENO, &rmtaddr);
    return 0;
}

int main(int argc, char **argv)
{
  testserve(argc, argv);
  return 0;
}

void processRequest(int clientfd)
{
  char buf[MAXLINE], headerName[MAXLINE], headerContent[MAXLINE];
  struct parsed_request rmtaddr;
  int parseStatus, bodyFlag;
  int serverfd;
  rio_t rio;

  dbg_printf("======Beginning of request processing...\n");
  /* Read the method, URI, version */
  Rio_readinitb(&rio, clientfd);
  Rio_readlineb(&rio, buf, MAXLINE); // ok this step removed buffer overflow

  /* Parse the fist line of request header */
  parseStatus = parse_first_line(buf, &rmtaddr);
  if ( parseStatus != 1)
  {
    clienterror(clientfd, buf, "501", "Input Format Error",
              "Error in the request URI format.");
    return;
  }
  dbg_printf("Remote name parsed:%s\nPath parsed:%s\nPortnumber:%d\n",
      rmtaddr.hostName, rmtaddr.path, rmtaddr.port);
  // Re-make the header sent to the remote
  // I'd like to make the
  serverfd = Popen_clientfd(rmtaddr.hostName, rmtaddr.port);
  if (serverfd < 0)
  {
    if (serverfd == -1)
      clienterror(clientfd, rmtaddr.uri, "500", "Internal Error",
              "Proxy can not connect to remote server.");
    else
      clienterror(clientfd, rmtaddr.uri, "404", "Not Found",
              "Proxy can not find the given remote address.");
    return;
  }

  // write the headers to the remote server
  write_req_hrds(serverfd, &rmtaddr);

  // write the rest of client request
  // by default no body included
  bodyFlag = 0;
  while (Rio_readlineb(&rio, buf, MAXLINE) > 0)
  {
    sscanf(buf, "%s %s", headerName, headerContent);
    if (!strcasecmp(headerName, "User-Agent:")
        || !strcasecmp(headerName, "accept:")
            || !strcasecmp(headerName, "accept-encoding:")
                || !strcasecmp(headerName, "Host:"))
    {
      continue;
    }
    if (!strcasecmp(headerName, "content-length: "))
    {
      bodyFlag = 1;
    }
    dbg_printf("outputing request header to remote...\n");
    Rio_writen(serverfd, buf, strlen(buf));
    if (strcmp(buf, "\r\n") == 0)
    {
      break;
    }
  }

  dbg_printf("====== After output the request headers...\n");

  // after a blank line,
  // write the rest of body of the client request (if any)
  if (bodyFlag)
  {
  dbg_printf("====== Caution: begin output the request body...\n");
  while (Rio_readlineb(&rio, buf, MAXLINE) && !strcmp(buf, "\r\n"))
  {
    dbg_printf("outputing request body to remote...\n");
    Rio_writen(serverfd, buf, strlen(buf));
  }
  dbg_printf("====== End of output request body ... \n");
  }

  // waiting for the server to return response
  int bytes = 0;
  int bytesread = 1;
  while((bytesread = rio_readn(serverfd, buf, MAXLINE))>0){
  //go until the stream is empty
  bytes += bytesread;
  rio_writen(clientfd,buf,bytesread);
  }
  close(serverfd);
  dbg_printf("====== End of request processing ... \n");
}

/**
 * Private Helper Functions
 */

  /* The uri should be an absolute addrjjess, must have ://
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

/**
 * parse_remote_address - Parse the hostname, port number and request path
 * from the URI part of the request.
 * This guy is assuming the uri not NULL, rmtaddr not NULL
 *
 * @return 1 if succeeful; 0 if failed
 */
static int parse_first_line(char *first_line, struct parsed_request *rmtaddr)
{
  char *uri, *hostBegin, *portNumBegin, *pathBegin;
  int hostlen, pathlen, portnumlen;

  if (sscanf(first_line, "%s %s %s", rmtaddr->method,
      rmtaddr->uri, rmtaddr->version) <= 0)
  {
    return 0;
  }
  uri = rmtaddr->uri;
  if ((hostBegin = strstr(uri, "://")) == NULL)
  {
    return 0;
  }
  /* Move to the begin of host */
  hostBegin += 3;
  if (((pathBegin = strstr(hostBegin, "/")) == NULL)
          || (pathBegin == hostBegin))
  {
    /*No valid path found*/
    return 0;
  }
  /* Find the beginning of port number */
  portNumBegin = strchr(hostBegin, ':') + 1;
  if ((portNumBegin - 1) == NULL || portNumBegin > pathBegin)
  {
    /* The colon is within the path */
    rmtaddr->port = 80;
    hostlen = (pathBegin - hostBegin);
    strncpy(rmtaddr->hostName, hostBegin, hostlen);
    if (hostlen < MAXLINE)
      (rmtaddr->hostName)[hostlen] = '\0';
  }else {
    /* The colon is there, port number exists, remove the colon as well */
    hostlen = (portNumBegin - hostBegin - 1);
    strncpy(rmtaddr->hostName, hostBegin, hostlen);
    if (hostlen < MAXLINE)
      (rmtaddr->hostName)[hostlen] = '\0';

    portnumlen = pathBegin - portNumBegin;
    char portNumStr[portnumlen];
    strncpy(portNumStr, portNumBegin, portnumlen);
    if ((rmtaddr->port = atoi(portNumStr)) == 0)
    {
      /* The port number format is invalid.*/
      return 0;
    }
  }

  /* Copy the path */
  pathlen = strlen(pathBegin);
  strncpy(rmtaddr->path, pathBegin, pathlen);
  if (pathlen < MAXLINE)
    (rmtaddr->path)[pathlen] = '\0';
  return 1;
}

/**
 * This routine will write the predefined headers to the fd specified
 */
void write_req_hrds(int outfd, struct parsed_request *rmtaddr)
{
	/**
	 * The http request should contain headers and body
	 * The header and body are separated by an extra CRLF
	 */
	char buf[MAXLINE];

	static char *line_separator = "\r\n";
  static char *host_hdr_name = "Host:";
	// write request line
	sprintf(buf, "%s %s %s%s", rmtaddr->method,
	    rmtaddr->path, HTTP_VERSION, line_separator);
	// write host line
	Rio_writen(outfd, buf, strlen(buf));
	if (rmtaddr->port == 80)
	{
	  // no need to add port number after host name
	  sprintf(buf, "%s %s%s", host_hdr_name, rmtaddr->hostName, line_separator);
	}else
	{
	  sprintf(buf, "%s %s:%d%s", host_hdr_name, rmtaddr->hostName,
	      rmtaddr->port, line_separator);
	}
	Rio_writen(outfd, buf, strlen(buf));

	// write the designated headers
	Rio_writen(outfd, (void *)USER_AGENT, strlen(USER_AGENT));
	Rio_writen(outfd, (void *)ACCEPT_ENCODING, strlen(ACCEPT_ENCODING));
	Rio_writen(outfd, (void *)ACCEPT_HDR, strlen(ACCEPT_HDR));

	return;
}
