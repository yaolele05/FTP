#include "event_loop.h"
#include <unistd.h>//close
#include <iostream>

void eventloop::addchannel(Channel*ch)
{
    channels[ch->fd]=std::shared_ptr<Channel>(ch);//??为什么不能直接赋值？将Channel对象的指针封装成shared_ptr，并存储在channels哈希表中，以便后续管理和调度事件
    epol.addfd(ch->fd,ch->events);

}

void eventloop::loop()
{
    while(1)
    {
        auto events=epol.poll(-1);

        for(auto &ev:events)
        {
          int fd=ev.data.fd;
          Channel* ch=channels[fd].get();

          if(ev.events & EPOLLIN)
          {
            if(ch->readcallback)
            {
                ch->readcallback();
            }
          }

          if(ev.events & EPOLLOUT)
          {
            if(ch->writecallback)
            {
                ch->writecallback();
            }
          }
        }

    }
}

