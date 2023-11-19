#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>


/*
    SERVER_IP "127.0.0.1"
    port_ 1313 
*/

void server_init(struct sockaddr_in *p, int port_){
    p -> sin_family = AF_INET;//ipv4
    p -> sin_port = htons(port_);
    p -> sin_addr.s_addr = htonl(INADDR_ANY);
}

int tcp_(char *server_ip, int server_port, char *data){
    int fd = -1;
    int ad_len = 0;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0){
        printf("tcp: socket create failed\n");
        return -1;
    }

    struct sockaddr_in server_ad;
    memset(&server_ad, 0, sizeof(server_ad));
    server_init(&server_ad, server_port);

    ad_len = sizeof(server_ad);
    int ret = connect(fd, (struct sockaddr *)&server_ad, ad_len);
    if (ret < 0) {
        printf("tcp: connect failed, errno:%d, %s\n", errno, strerror(errno));
        return -1;
    }

    write(fd, data, strlen(data));
    printf("send data to server: %s\n", data);

    ret = read(fd, buf, sizeof(buf));
    if (ret > 0) {
        printf("receive data: %s\n", buf);
    } else if (ret < 0) {
        printf("tcp: errno:%d\n", errno);
    } else if (0 == ret) {
        printf("server %d disconnect\n", fd);
    }

}

int main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("example: ./client 127.0.0.1 1313 helloworld\n");
        return -1;
    }

    int flag = tcp_(argv[1], atoi(argv[2]), argv[3]);
    switch (flag)
    {
        case 0:
            printf("client shutdown...\n");
            break;
        case -1: 
            printf("error\n");
            break;
        default:
            break;
    }

    return 0;
}
