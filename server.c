#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

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

void server_init(struct sockaddr_in *p){
    p -> sin_family = AF_INET; //ipv4
    p -> sin_port = htons(port_);
    p -> sin_addr.s_addr = htonl(INADDR_ANY);//Разрешить подключение по любому ip-адресу
}

void save_value(char *s){//После этого добавьте операцию хранения здесь
    FILE* f = fopen("1.save", "a");//Пополнить
    fprintf(f, "%s\n", s);
    fclose(f);
}

int rest = 1;
int fd_server = -1;//Идентификатор сервера
int ad_len;

int get(char *buf, int fd_client){
    int ans = read(fd_client, buf, 1024);
    printf("!????? %d %d %d\n", fd_client, buf, ans);
    return ans;
}


// void *talk(int fd_client, int fd_server, struct sockaddr_in client_ad, int ad_len){
void *talk(void *fd_client_){//Взаимодействие с клиентами
    int fd_client = (int) fd_client_;
    printf("fd_client %d\n", fd_client);
    char buf[1024];//читать буфер
    memset(buf, 0, sizeof(buf));
    // fd_client = accept(fd_server, (struct sockaddr *)&client_ad, &ad_len);
    if(fd_client < 0){
        printf("tcp: fail to accept because of invalid client fd %d\n", fd_client);
        return -1;
    }
    while(1){
        if(rest >= 0){
            rest --;//Ограничение с семафором
            break;
        }
    }
    printf("--- client %d connect\n", fd_client);

    while(1){
        // int read_l = read(fd_client, buf, 1024);
        // int read_l = recv(fd_client, &buf, sizeof(buf), 0);
        int read_l = get(buf, fd_client);
        if(read_l < 0) {
            printf("error while reading\n");
            break;
        } 
        if(read_l == 0) {//Чтение завершено
            printf("--- Complete reception, client %d disconnect\n", fd_client);
            break;
        }
        
        printf("receive data from client %d: %s\n", fd_client, buf);

        save_value(buf);

        char opt[100];//Чтение типа команды
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

        write(fd_client, buf, read_l);//Возврат информации о результате операции клиенту
        printf("send data to client %d: %s\n", fd_client, buf);
    }
    close(fd_client);
    rest ++;//Возврат ресурсов
    pthread_exit(0);
}

int tcp_(){ 
    //Связь по протоколу TCP    
    fd_server = socket(AF_INET, SOCK_STREAM, 0);//ipv4, tcp
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
    server_init(&server_ad);//Настройки инициализации

    int ad_len = sizeof(server_ad);
    if(bind(fd_server, (const struct sockaddr *)&server_ad, ad_len) < 0){//Привязать адрес
        printf("tcp: fail to bind\n");
        return -1;
    }
    int flag = listen(fd_server, 5);
    if(flag < 0){
        printf("tcp: fail to listen\n");
        return -1;
    }
    pthread_t thread_id[100];
    int thread_tot = -1;
    int fd_client;//идентификатор клиента
    while(1){//Многопоточность будет добавлена ​​сюда позже.
        if(thread_tot >= 99) thread_tot = -1;//Ограничить лимит клиентских подключений
        // int fd_client = -1;
        fd_client = accept(fd_server, (struct sockaddr *)&client_ad, &ad_len);        

        int ret_thread = pthread_create(&thread_id[thread_tot], NULL, talk, (void *)fd_client);
		if(-1 == ret_thread) printf("thread failed"); 
    }
    close(fd_server);
   
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


