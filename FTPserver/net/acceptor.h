#pragma once
#include "channel.h"
#include "eventloop.h"

#include <functional>
class Acceptor
{
    public:
    using Newconncallback=std::function<void(int)>;

    Acceptor(eventloop*loop,int port);
    ~Acceptor();
    void setnewconncallback( Newconncallback cb);

    private:
    void handleread();
    void setnonblock(int fd);
    private:
    int  listenfd;
     eventloop* loop;
     Channel* acceptchannel;
     Newconncallback newconncallback;
};
   