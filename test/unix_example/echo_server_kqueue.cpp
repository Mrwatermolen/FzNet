#include <netinet/in.h>
#include <sys/_types/_ssize_t.h>
#include <sys/_types/_timespec.h>
#include <sys/_types/_timeval.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[]) {
  std::cerr << "Not implemented\n";
  return 1;

  uint16_t port = 2314;

  if (argc > 1) {
    port = std::stoi(argv[1]);
  }

  int serv_sock;
  int clnt_sock;
  struct sockaddr_in serv_addr;
  struct sockaddr_in clnt_addr;
  socklen_t adr_sz;
  char buf[512];

  serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);
  if (bind(serv_sock, reinterpret_cast<struct sockaddr*>(&serv_addr),
           sizeof(serv_addr)) == -1) {
    std::cerr << "bind() error\n";
    return 1;
  }

  if (listen(serv_sock, 5) == -1) {
    std::cerr << "listen() error\n";
    return 1;
  }

  int kq = kqueue();  // file descriptor for kqueue
  if (kq == -1) {
    std::cerr << "kqueue() error\n";
    return 1;
  }

  struct kevent ev_set;  // event to be monitored

  EV_SET(&ev_set, serv_sock, EVFILT_READ, EV_ADD, 0, 0,
         nullptr);  // add serv_sock to monitor read events
  // register ev_set to kqueue
  if (kevent(kq, &ev_set, 1, nullptr, 0, nullptr) == -1) {
    std::cerr << "kevent() error\n";
    return 1;
  }

  struct timespec timeout;
  struct kevent ev_list[10];  // triggered events
  while (true) {
    // wait for events
    timeout.tv_sec = 5;   // 5 seconds
    timeout.tv_nsec = 0;  // 0 nanoseconds

    // Query the triggered events
    int event_cnt = kevent(kq, nullptr, 0, ev_list, 10, &timeout);
    if (event_cnt == -1) {
      std::cerr << "kevent() error\n";
      break;
    }

    if (event_cnt == 0) {
      std::cout << "timeout\n";
      continue;
    }

    for (int i = 0; i < event_cnt; ++i) {
      if ((ev_list[i].flags & EV_ERROR) != 0) {
        std::cerr << "Error. fd: " << ev_list[i].ident << "\n";
        return 1;
      }

      if (ev_list[i].filter == EVFILT_WRITE) {
        const char* message = "Data received. Echoing back!\n";
        write(ev_list[i].ident, message, strlen(message));
        EV_SET(&ev_set, ev_list[i].ident, EVFILT_WRITE, EV_DELETE, 0, 0,
               nullptr);
        continue;
      }

      if (ev_list[i].ident ==
          static_cast<decltype(ev_list[i].ident)>(serv_sock)) {
        // handle new connection
        adr_sz = sizeof(clnt_addr);
        clnt_sock = accept(
            serv_sock, reinterpret_cast<struct sockaddr*>(&clnt_addr), &adr_sz);
        if (clnt_sock == -1) {
          std::cerr << "accept() error\n";
          continue;
        }

        // register clnt_sock to monitor read events
        int flags = fcntl(clnt_sock, F_GETFL, 0);       // get file status flags
        fcntl(clnt_sock, F_SETFL, flags | O_NONBLOCK);  // set non-blocking mode
        EV_SET(&ev_set, clnt_sock, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        if (kevent(kq, &ev_set, 1, nullptr, 0, nullptr) == -1) {
          std::cerr << "kevent() error\n";
          return 1;
        }

        std::cout << "New connection. Client fd: " << clnt_sock << '\n';
        continue;
      }

      ssize_t len = read(ev_list[i].ident, buf, sizeof(buf));
      if (len == 0 || len == -1) {
        // close connection
        std::cout << "Client: " << ev_list[i].ident << " close connection.\n";
        EV_SET(&ev_set, ev_list[i].ident, EVFILT_READ, EV_DELETE, 0, 0,
               nullptr);
        kevent(kq, &ev_set, 1, nullptr, 0, nullptr);
        close(ev_list[i].ident);
        continue;
      }

      std::cout << "Get message: " << buf << " from client " << ev_list[i].ident
                << '\n';
      EV_SET(&ev_set, ev_list[i].ident, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
    }
  }
}