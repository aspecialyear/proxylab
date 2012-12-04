#include "csapp.h"
#include "proxylhelpers.h"
#include "proxyservant.h"

#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf("Thread #%lu dgb:", \
    pthread_self()); printf(__VA_ARGS__);
#else
# define dbg_printf(...)
#endif

static const char *USER_AGENT = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *ACCEPT_HDR = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *ACCEPT_ENCODING = "Accept-Encoding: gzip, deflate\r\n";
static const char *CONNECTION_HDR = "Connection: close";
static const char *PROXY_CONN_HDR = "Proxy-Connection: close";
static const char *HTTP_VERSION = "HTTP/1.0";

void processRequest(int clientfd)
{
  char buf[MAXLINE], headerName[MAXLINE], headerContent[MAXLINE];
  struct parsed_request rmtaddr;
  int parseStatus, bodyFlag, serverfd, contentlen, bytes, bytesread;
  rio_t clientrio, serverrio;

  dbg_printf("======Beginning of request processing...\n");
  /* Read the method, URI, version */
  Rio_readinitb(&clientrio, clientfd);
  MRio_readlineb(&clientrio, buf, MAXLINE); // ok this step removed buffer overflow

  /* Parse the fist line of request header */
  parseStatus = parse_first_line(buf, &rmtaddr);
  if ( parseStatus != 1)
  {
    clienterror(clientfd, buf, "501", "Input Format Error",
              "Error in the request URI format.");
    return;
  }
  dbg_printf("Request line: %s %s:%d%s",
      rmtaddr.method, rmtaddr.hostName, rmtaddr.port, rmtaddr.path);

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
  while (1)
  {
    MRio_readlineb(&clientrio, buf, MAXLINE);
    sscanf(buf, "%s %s", headerName, headerContent);
    if (!strcasecmp(headerName, "User-Agent:")
        || !strcasecmp(headerName, "accept:")
            || !strcasecmp(headerName, "accept-encoding:")
                || !strcasecmp(headerName, "Host:")
                    || !strcasecmp(headerName, "Connection:")
                      || !strcasecmp(headerName, "Proxy-Connection:"))
    {
      continue;
    }
    if (!strcasecmp(headerName, "content-length:"))
    {
      dbg_printf("Detected request with content-length.\n");
      if ((contentlen = atoi(headerContent)) == 0)
      {
        clienterror(clientfd,buf,"501","Illegal content length",
            "The request header contains illegal content length.\n");
        return;
      }else {
        bodyFlag = 1;
        dbg_printf("Content length read: %d", contentlen);
      }
    }
    dbg_printf("outputing request header to remote...\n");
    MRio_writen(serverfd, buf, strlen(buf));
    dbg_printf("The output is [%s]\n", (char *)buf);

    if (strcmp(buf, "\r\n") == 0)
    {
      dbg_printf("Break at the end of request header.\n");
      break;
    }
  }

  dbg_printf("====== After output the request headers...\n");

  // after a blank line,
  // write the rest of body of the client request (if any)
  // TODO Add condition that know the size of the request body
  if (bodyFlag)
  {
  dbg_printf("====== Caution: begin output the request body...\n");
  while (bytes < contentlen && 0)
  {
    MRio_readnb(&clientrio, buf, MAXLINE, bytesread);
    bytes += bytesread;
    dbg_printf("outputing request body to remote...\n");
    MRio_writen(serverfd, buf, bytesread);
  }
  /* Reset the bytes read.*/
  bytes = 0;
  dbg_printf("====== End of output request body ... \n");
  } else {
    MRio_writen(serverfd, "\r\n", 2);
  }

  // waiting for the server to return response
  bytesread = 1;
  // initiate the buffer for server read
  Rio_readinitb(&serverrio, serverfd);

  dbg_printf("====== Begin copy server response to client\n");
  while(1){
    dbg_printf("Reading response from server...\n");
  MRio_readnb(&serverrio, buf, MAXLINE, bytesread);
    //bytesread = Rio_readnb(&serverrio, buf, MAXLINE);
  dbg_printf("Read %d bytes from server.\n", bytesread);
  //go until the stream is empty
  if (bytesread == 0)
    break;
  bytes += bytesread;
  MRio_writen(clientfd, buf, bytesread);
  }
  dbg_printf("====== Server response sent to client, closing server fd...\n");
  close(serverfd);
  dbg_printf("====== End of request processing ... \n");
}

/**
 * parse_remote_address - Parse the hostname, port number and request path
 * from the URI part of the request.
 * This guy is assuming the uri not NULL, rmtaddr not NULL
 *
 * @return 1 if succeeful; 0 if failed
 */
int parse_first_line(char *first_line, struct parsed_request *rmtaddr)
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

  // write request line separately
  sprintf(buf, "%s %s %s\r\n", rmtaddr->method,
      rmtaddr->path, HTTP_VERSION);
  MRio_writen(outfd, buf, strlen(buf));

  if (rmtaddr->port == 80)
  {
    // no need to add port number after host name
    sprintf(buf, "Host: %s", rmtaddr->hostName);
  }else
  {
    sprintf(buf, "Host: %s:%d", rmtaddr->hostName,
        rmtaddr->port);
  }

  // write the designated headers
  sprintf(buf, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n\r\n",
      buf, USER_AGENT, ACCEPT_HDR, ACCEPT_ENCODING,
      CONNECTION_HDR, PROXY_CONN_HDR);
  MRio_writen(outfd, buf, strlen(buf));
  return;
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
