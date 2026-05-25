#include "eventloop.h"
#include "acceptor.h"
#include "connection.h"

#include <unordered_map>

int main()
{
    eventloop loop;

    std::unordered_map<int,Connection*> conns;

    Acceptor acceptor(&loop,2100);

    acceptor.setnewconncallback(
        [&](int connfd)
        {
            Connection* conn=
                new Connection(&loop,connfd);

            conns[connfd]=conn;
        }
    );

    loop.loop();

    return 0;
}