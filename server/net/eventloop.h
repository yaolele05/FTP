#pragma once 
#include "epoll.h"
#include <unordered_map>

class Channel;

class eventloop
{
    public:
    eventloop();
    ~eventloop();

    void loop();
    void addchannel(Channel* ch);
    void updatechannel(Channel* ch);
    void removechannel(Channel* ch);

    private:
    Epoll epoll;
    std::unordered_map<int,Channel*> channels;
};