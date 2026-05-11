#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

int main()
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listenfd, (sockaddr*)&addr, sizeof(addr));
    listen(listenfd, 10);

    int epfd = epoll_create1(0);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

    epoll_event events[1024];

    while (true)
    {
        int n = epoll_wait(epfd, events, 1024, -1);

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].data.fd;

            if (fd == listenfd)
            {
                int connfd = accept(listenfd, nullptr, nullptr);

                epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = connfd;

                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

                std::cout << "new client\n";
            }
            else
            {
                char buf[1024];
                int n = recv(fd, buf, sizeof(buf), 0);

                if (n <= 0)
                {
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                }
                else
                {
                    send(fd, buf, n, 0);
                }
            }
        }
    }
}