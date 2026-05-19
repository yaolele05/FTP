#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#include "reactor/channel.h"
#include "reactor/epoll.h"
#include "reactor/event_loop.h"

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listenfd, (sockaddr*)&addr, sizeof(addr));
    listen(listenfd, 128);

    eventloop loop;

    Channel listenCh(listenfd);
    listenCh.events = EPOLLIN;

    listenCh.setreadcallback([&]() {
        sockaddr_in client;
        socklen_t len = sizeof(client);

        int connfd = accept(listenfd, (sockaddr*)&client, &len);

        std::cout << "new connfd: " << connfd << std::endl;

        Channel* connCh = new Channel(connfd);
        connCh->events = EPOLLIN;

        connCh->setreadcallback([=]() {
            char buf[1024];
            int n = read(connfd, buf, sizeof(buf));

            if (n <= 0) {
                close(connfd);
                return;
            }

            write(connfd, buf, n);
        });

        loop.addchannel(connCh);
    });

    loop.addchannel(&listenCh);

    loop.loop();
}