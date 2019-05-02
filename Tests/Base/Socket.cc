#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include <Unittest.h>

#define PORT 8080
#define BACKLOG 1

TEST(Socket, Bind) {
  Int sfd = -1, opt = 1;
  sockaddr_in addr;
  socklen_t len;

  if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    throw Except(EWatchErrno, Base::Format{"socket: {}"} << strerr(errno));
  }

  if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
    throw Except(EWatchErrno, Base::Format{"setsockopt: {}"} << strerr(errno));
  }

  memset(&addr, 0, sizeof(sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sfd, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1) {
    throw Except(EWatchErrno, Base::Format{"bind: {}"} << strerr(errno));
  }

  if (listen(sfd, BACKLOG) == -1) {
    throw Except(EWatchErrno, Base::Format{"socket: {}"} << strerr(errno));
  }

  close(sfd);
}

TEST(Socket, Communicate) {
  auto perform = [&]() {
    Base::Thread server, client;

    client.Start([]() {
      String buff = "hello from client";
      sockaddr_in addr;
      Int std;
      sleep(10);

      if ((sfd = socket(AF_INET, SOCK_STREAM, 0))) {
        throw Except(EWatchErrno, Base::Format{"socket: {}"} << strerr(errno));
      }

      bzero(&addr, sizeof(addr));

      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr("127.0.0.1");
      addr.sin_port = htons(PORT);

      if (connect(sfd, (sockaddr*)&addr, sizeof(addr)) == -1) {
        throw Except(EWatchErrno, Base::Format{"connect: {}"} << strerr(errno));
      }

      send(sfd, buff, sizeof(buff), 0);
      //close(sfd);
    });

    server.Start([]() {
      int sfd = -1, cfd = -1, opt = 1;
      sockaddr_in addr;
      socklen_t len;

      if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        _exit(-1);
      }

      if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
        _exit(-1);
      }

      memset(&addr, 0, sizeof(sockaddr_in));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(PORT);
      addr.sin_addr.s_addr = INADDR_ANY;

      if (bind(sfd, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1) {
        throw Except(EWatchErrno, Base::Format{"bind: {}"} << strerr(errno));
      }

      if (listen(sfd, BACKLOG) == -1) {
        throw Except(EWatchErrno, Base::Format{"listen: {}"} << strerr(errno));
      }

      for (int i = 0; i < 1; i++) {
        Char buff;

        if ((cfd = accept(sfd, (sockaddr*)&addr, &len)) < 0) {
          continue;
        }

        while(recv(sfd, &buff, 1, 0) > 0) { sleep(1); }
        shutdown(cfd, 1);
      }

      close(sfd);
    });
  };

  TIMEOUT(50, { perform(); });
}

int main() {
  return RUN_ALL_TESTS();
}
