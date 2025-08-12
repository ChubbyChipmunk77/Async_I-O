#include <arpa/inet.h>
#include <malloc.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

enum EventType { EventTypeMessage };

struct Request {
  struct sockaddr client_addr;
  socklen_t addr_size;
  int server_sockfd;
  char *msg;
};

struct Server {
  int port;
  int sockfd;
  void(*cb_array[EVENT_TYPES_LEN](struct Request *));
};

struct Server *createServer(int port);
void on(struct Server *server, enum EventType event,
        void (*callback)(struct Request *Request));
void startServer(struct Server *server);

void on(struct Server *server, enum EventType event,
        void (*callback)(struct Request *Request)) {
  server->cb_array[event] = callback;
}

void startServer(struct Server *server) {
  int epollfd = epoll_create(0);

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = server->sockfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, server->sockfd, &ev);
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

int main() {
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
}
