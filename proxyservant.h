/*
 * proxyservant.h
 *
 *  Created on: Dec 3, 2012
 *      Author: xinkaihe
 */

#ifndef PROXYSERVANT_H_
#define PROXYSERVANT_H_

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

void processRequest(int connfd);
int parse_first_line(char *first_line, struct parsed_request *rmtaddr);
void write_req_hrds(int outfd, struct parsed_request *rmtaddr);


#endif /* PROXYSERVANT_H_ */
