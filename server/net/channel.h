#pragma once

#include <functional>
#include <sys/epoll.h>

class Channel
{
public:
    Channel(int fd_);
    ~Channel();
    void setreadcallback(std::function<void()> cb);

    void setwritecallback(std::function<void()> cb);

    void handleevent();



public:
    int fd;

    uint32_t events;
    uint32_t revents;  
    std::function<void()> readcallback;

    std::function<void()> writecallback;
};