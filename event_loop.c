#include <arpa/inet.h>
#include <malloc.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define BUF_SIZE 1024
#define EVENT_TYPES_LEN 1

typedef enum EventType { EventTypeMessage } EventType;

typedef struct Request {
  struct sockaddr client_addr;
  socklen_t addr_size;
  int server_sockfd;
  char *msg;
  size_t msg_len;
} Request;

typedef struct Server {
  int port;
  int sockfd;
  void (*cb_array[EVENT_TYPES_LEN])(struct Request *);
} Server;

Server *createServer(int port);
void on(Server *server, EventType event, void (*callback)(Request *Request));
void startServer(Server *server);

void on(Server *server, EventType event, void (*callback)(Request *Request)) {
  server->cb_array[event] = callback;
}

void handleMessage(Request *req) {
  if (sendto(req->server_sockfd, req->msg, req->msg_len, 0, &req->client_addr,
             req->addr_size) == -1) {
    perror("send");
    exit(EXIT_FAILURE);
  }
}

void startServer(Server *server) {
  int epollfd = epoll_create1(0);

  struct epoll_event ev, events[MAX_EVENTS];
  ev.events = EPOLLIN;
  ev.data.fd = server->sockfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, server->sockfd, &ev);

  printf("Listening on port %d\n", server->port);
  // Here starts the event loop

  while (1) {
    int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nfds; i++) {
      if (events[i].events & EPOLLIN && events[i].data.fd == server->sockfd) {
        struct sockaddr client_addr;
        char buf[BUF_SIZE];
        socklen_t addr_size = sizeof(client_addr);
        memset(buf, 0, BUF_SIZE);
        int bytes_read;
        bytes_read = recvfrom(server->sockfd, buf, BUF_SIZE, 0, &client_addr,
                              &addr_size);
        if (bytes_read == -1) {
          perror("recv");
          exit(EXIT_FAILURE);
        }
        Request req = {0};
        req.client_addr = client_addr;
        req.addr_size = addr_size;
        req.msg = buf;
        req.msg_len = (size_t)bytes_read;
        req.server_sockfd = server->sockfd;

        server->cb_array[EventTypeMessage](&req);
      }
    }
  }
}

int createUDPsocket() {
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  return sockfd;
}

struct sockaddr_in *createServerSocketAddr(char *ip, int port) {
  struct sockaddr_in *serverSockAddr = malloc(sizeof(struct sockaddr_in));
  serverSockAddr->sin_port = htons(port);
  serverSockAddr->sin_family = AF_INET;
  if (strlen(ip) == 0) {
    serverSockAddr->sin_addr.s_addr = INADDR_ANY;
  } else {
    inet_pton(AF_INET, ip, (void *)&serverSockAddr->sin_addr.s_addr);
  }
  return serverSockAddr;
}

Server *createServer(int port) {
  int serverSockFD = createUDPsocket();
  struct sockaddr_in *serverSockAddr = createServerSocketAddr("", 2000);
  printf("Server socket created...");

  int res = bind(serverSockFD, (const struct sockaddr *)serverSockAddr,
                 sizeof *serverSockAddr);
  if (res < 0) {
    printf("Failed to bind the socket to the local address !!!");
  } else {
    printf("Socket successfully binded to the local Address");
  }

  free(serverSockAddr);
  Server *createdServer = calloc(1, sizeof(Server));
  createdServer->port = port;
  createdServer->sockfd = serverSockFD;
  return createdServer;
}

int main() {
  Server *createdServer = createServer(2000);
  on(createdServer, EventTypeMessage, handleMessage);
  startServer(createdServer);
}
