#pragma once
#include <vector>
#include <sys/epoll.h>
class Channel;

class Epoll
{
    public:
    Epoll();
    ~Epoll();

    void addchannel(Channel*ch);
    void modchannel(Channel*ch);
    void delchannel(Channel*ch);
    std::vector<Channel*> poll(int timeout);//channel和epoll_event的关系?
    private:
    int epfd;
   std::vector<epoll_event> events;

};