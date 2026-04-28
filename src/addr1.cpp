#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>
/*int main()
{
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(8080);
    inet.pton(AF_INET,"127.0.0.1",&addr.sin.addr);

    printf("IPv4 address:%s\n",inet_ntoa(addr.sin_addr));
    return 0;
}
 */
int main()
{
    struct sockaddr_un addr;
    addr.sun_family=AF_UNIX;
    strncpy(addr.sun_path,"/tmp/unix/socket",sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path)-1]='\0';
    printf("UNIX domain socket path:%s\n",addr.sun_path);
    return 0;
    
}