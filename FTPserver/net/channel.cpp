#include "channel.h"
#include <iostream>
Channel::Channel(int fd_)
    :fd(fd_),
     events(0)
{
}

void Channel::setreadcallback(std::function<void()> cb)
{
    readcallback=cb;
}

void Channel::setwritecallback(std::function<void()> cb)
{
    writecallback=cb;
}

void Channel::handleevent()
{
    std::cout<<"channel handleevent"<<std::endl;
    if(revents & EPOLLIN)
    {
        if(readcallback)
        {
            readcallback();
        }
    }

    if(revents & EPOLLOUT)
    {
        if(writecallback)
        {
            writecallback();
        }
    }
    
}
Channel::~Channel()
{
}