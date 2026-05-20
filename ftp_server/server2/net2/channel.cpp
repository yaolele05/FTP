#include "channel.h"

Channel::Channel(int fd):fd(fd),events(0){}
int Channel::getfd() const{
    return fd;
}

uint32_t Channel::getevents()const
{
    return events;

}

void Channel::setevents(uint32_t ev)
{
    events=ev;
}

void Channel::enableread()
{
    events|=EPOLLIN;    
}

void Channel::enablewrite()
{
    events|=EPOLLOUT;
}

void Channel::disablewrite()
{
    events&=~EPOLLOUT;
}

void Channel::setreadcallback(Callback cb)
{
    readcallback=cb;
}
void Channel::setwritecallback(Callback cb)
{
    writecallback=cb;
}
void Channel::handleevent(uint32_t revents)
{
    if((revents & EPOLLIN) && readcallback)
    {
        readcallback();
    }
    if((revents & EPOLLOUT) && writecallback)
    {
        writecallback();
    }
}