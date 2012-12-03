/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content.
 */
#include "csapp.h"
#include <stdio.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection = "Connection: close\r\n";
static const char *proxy_connection = "Proxy-Connection: close\r\n";

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
  int listenfd, clientfd, port, serverfd;
  int tempport;
  char buf[MAXLINE];
  char buf2[MAXLINE];
  char hostnm[MAXLINE];
  char pathnm[MAXLINE];
  char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char proxySend[MAXLINE];
  struct sockaddr_in clientaddr;
  rio_t rio;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);

    while (1) {
	clientlen = sizeof(clientaddr);
	clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        
	Rio_readinitb(&rio, clientfd);
        Rio_readlineb(&rio, buf, MAXLINE);

	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET")) {
	  clienterror(fd, method, "501", "Not Implemented",
		   "Proxy does not implement this method");
	  exit(0);
	}

	parse_uri(&uri,&hostnm, &pathnm, &tempport);

        serverfd = Open_clientfd(&hostnm,tempport);
	sprintf(buf2,"GET /%s HTTP/1.0\r\n Host:%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n",pathnm,
                hostnm,user_agent,accept,accept_encoding,connection,proxy_connection);
	Rio_writen(serverfd,buf2,strlen(buf2));

        int bytes = 0;
	int bytesread = 1;
	while((bytesread = Rio_readnb(serverfd,(void *)&buf,MAXLINE))>0){
	  //go until the stream is empty
	  bytes += bytesread;
	  Rio_writenb(clientfd,buf,bytesread);
	}

	/* close the fds*/
        Close(serverfd);
	Close(clientfd);
    }
}


/*
 * parse_uri - URI parser
 *
 * Given a URI from the HTTP proxy GET request, extract the host name,
 * path name, and port. Return -1 if there are any problems.
 */

int parse_uri(char *uri, char *hostname, char *pathname, int *port) 
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;
 
    if(strncasecmp(uri,"http://",7)!= 0){
      hostname[0] = '\0';
      return -1;
    }

    /*Extract the host name*/

    hostbegin = uri + 7;
    /*Returns a string starting from the character found*/
    hostend = strpbrk(hostbegin, ":/\r\n\0"); 
    len = hostend - hostbegin;
    strncpy(hostname,hostbegin,len);
    hostname[len] = '\0';



    /*Extract the port number*/
    *port = 80; /*default*/
    if(*hostend = ':')
      *port = atoi(hostend+1);

    /*Extract the path*/
    pathbegin = strchr(hostbegin,'/');
    if(pathbegin == NULL){
      pathname[0] = '\0';
    }
    else {
      pathbegin++;
      strcpy(pathname,pathbegin);
    }

    return 0;

]



/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end clienterror */
