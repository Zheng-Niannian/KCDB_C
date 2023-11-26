#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#define port_ 1313
const unsigned int reuseable = 1;
const char help_[] = {"help"};
const char save_[] = {"save"};
const char find_[] = {"find"};
const char savefile_[] = {"savefile"};
const char load_[] = {"load"};

bool check(const char *a, const char *b){
    int len_a = strlen(a);
    int len_b = strlen(b);
    printf("checking %s %s\n", a, b);
    if(len_a != len_b) return false;
    int i;
    for(i = 0; i < len_a; i ++){
        if(a[i] != b[i]) return false;
    }
    return true;
}

int get_type(char *s){
    printf("get type of %s\n", s);
    if(check(help_, s)) return 0;
    return -1;
}

void server_init(struct sockaddr_in *p)
{
    p -> sin_family = AF_INET;//ipv4
    p -> sin_port = htons(port_);
    p -> sin_addr.s_addr = htonl(INADDR_ANY);
}

void save_value(char *s){
    FILE* f = fopen("1.save", "a");
    fprintf(f, "%s\n", s);
    fclose(f);
}
int tcp_(){
    
    int fd_server = -1;
    int fd_client = -1;
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    fd_server = socket(AF_INET, SOCK_STREAM, 0);//用ipv4, tcp协议
    if(fd_server < 0){
        printf("tcp: socket create failed\n");
        return -1;
    }
    
    if(setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, (void *)& reuseable, sizeof(reuseable)) < 0){
        printf("tcp: fail to reuse\n");
        return -1;
    }

    struct sockaddr_in server_ad, client_ad;
    memset(&server_ad, 0, sizeof(server_ad));
    memset(&client_ad, 0, sizeof(client_ad));
    server_init(&server_ad);

    int ad_len = sizeof(server_ad);
    if(bind(fd_server, (const struct sockaddr *)&server_ad, ad_len) < 0){
        printf("tcp: fail to bind\n");
        return -1;
    }
    int flag = listen(fd_server, 5);
    if(flag < 0){
        printf("tcp: fail to listen\n");
        return -1;
    }
    while(1){
        fd_client = accept(fd_server, (struct sockaddr *)&client_ad, &ad_len);
        if(fd_client < 0){
            printf("tcp: fail to accept because of invalid client fd %d\n", fd_client);
            return -1;
        }
        printf("--- client %d connect\n", fd_client);

        while(1){
            int read_l = read(fd_client, buf, 1024);
            if(read_l < 0) {
                printf("error while reading\n");
                break;
            } 
            if(read_l == 0) {
                printf("--- Complete reception, client %d disconnect\n", fd_client);
                break;
            }
  
            printf("receive data from client %d: %s\n", fd_client, buf);

            save_value(buf);
            char opt[100];
            int i, tot = 0; int len = strlen(buf);
            for(i = 0; i < len; i ++){
                if((buf[i] >= 'a' && buf[i] <= 'z') || (buf[i] >= 'A' && buf[i] <= 'Z') || (buf[i] >= '0' && buf[i] <= '9')){
                    opt[tot ++] = buf[i];
                } else break; 
            }

            switch (get_type(opt))
            {
                case 0:
                    printf("------------------------------------------------------\n");
                    printf("help\n");
                    printf("------------------------------------------------------\n");
                    break;
         
                default:
                    printf(">unknow operation\n");
                    break;
            }                 
            write(fd_client, buf, read_l);
            printf("send data to client %d: %s\n", fd_client, buf);
        }

    }
    close(fd_server);
    close(fd_client);
    return 0;
}

int main(){
    int flag = tcp_();
    switch (flag)
    {
        case 0:
            printf("server shutdown...\n");
            break;
        case -1: 
            printf("socket error\n");
            break;
        default:
            break;
    }
}


