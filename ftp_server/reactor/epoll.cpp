#include "epoll.h"
#include <unistd.h>

epoll::epoll()
{
    epfd=epoll_create1(0);

}

void epoll::addfd(int fd,uint32_t events)
{
    epoll_event ev;
    ev.events=events;
    ev.data.fd=fd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev);
}

void epoll::modfd(int fd,uint32_t events)
{
    epoll_event ev;
    ev.events=events;
    ev.data.fd=fd;
    epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
}
void epoll::delfd(int fd)
{
    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,nullptr);
    //close(fd);
}

std::vector<epoll_event>epoll::poll(int timeout)
{
    std::vector<epoll_event> events(1024);
    int n=epoll_wait(epfd,events.data(),events.size(),timeout);
    //epoll_wait返回就绪事件的数量，如果n>0，说明有事件发生，我们可以根据n来调整events向量的大小，以便只处理实际发生的事件。
    if(n>0)
    {
        events.resize(n);
    }
    //如果n<=0，说明没有事件发生或者发生了错误，此时我们可以选择清空events向量，以避免处理无效的事件。
    else
    {
        events.clear();
    }
    return events;
}