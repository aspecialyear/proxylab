/**
 * Proxy Lab
 *
 * - Team -
 * Chong Ren * cren@andrew.cmu.edu
 * Xinkai He * xinkaihe@andrew.cmu.edu
 */
#include "proxylhelpers.h"
#include "proxyservant.h"
#include "csapp.h"
#include "sbuf.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define THREAD_POOL_SIZE 4
#define SBUF_SIZE 16

#define MAX_HEADER_SIZE 8192

#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

int testserve(int argc, char **argv);
void *thread(void *vargp);

sbuf_t sbuf; /* Shared buffer of client descriptors */

int main(int argc, char **argv)
{
  /* Register SIG_IGN to SIGPIPE and SIGSEGV signal */
  Signal(SIGPIPE, SIG_IGN);
  Signal(SIGSEGV, SIG_IGN);

  testserve(argc, argv);
  return 0;
}

int testserve(int argc, char **argv)
{
  int i, listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;
  pthread_t tid;

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);
  listenfd = Open_listenfd(port);
  sbuf_init(&sbuf, THREAD_POOL_SIZE);

  printf("Listening on port:%d...\n", port);

  for (i = 0; i < THREAD_POOL_SIZE; i++)
  {
    Pthread_create(&tid, NULL, thread, NULL);
  }

  dbg_printf("Thread pool initialized.\n");
  while(1)
  {
    dbg_printf("Server waiting for connection...\n");
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
      (unsigned int *) (&clientlen));
    dbg_printf("Accepted new client with fd:%d\n", connfd);
    sbuf_insert(&sbuf, connfd);
    dbg_printf("Inserted into the sbuf.\n");
  }
  Close(listenfd);
  return 0;
}

/**
 * thread - Pool worker working routine
 */
void *thread(void *arg)
{
  /** If the thread pool init failed, should exit */
  pthread_t pid = pthread_self();
  Pthread_detach(pid);
  dbg_printf("Thread #%lu initialized and detached.\n", pid);
  while (1)
  {
    dbg_printf("Thread #%lu waiting for job...\n", pid);
    int connfd = sbuf_remove(&sbuf);
    dbg_printf("Thread #%lu get job, processing...\n", pid);
    processRequest(connfd);
    dbg_printf("Thread #%lu processing complete, closing connection...", pid);
    myClose(connfd);
    dbg_printf("connection closed, return\n");
  }
  return NULL;
}
