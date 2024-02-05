#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>  
#include <semaphore.h>

pthread_t thread_id[100];
int thread_tot = -1;

void *talk(void *num) {//Взаимодействие с клиентом.
    intptr_t num_ = (intptr_t) num;  
    printf("!!%ld\n", num_);  

    char ans[100] = "./client";
    char ss[2];
    ss[0] = num_ + '0';
    ss[1] = 0;
    strcat(ans, ss);
    strcat(ans, " 127.0.0.1 1313 < ");
    strcat(ans, ss);
    strcat(ans, ".in > ");
    strcat(ans, ss);
    strcat(ans, ".out");
    system(ans);

    printf("%ld: %s\n", num_, ans);

    pthread_exit(NULL);
}

int ss[10] = {0, 1, 2, 0, 0, 1, 2};

int main() {
    for (int i = 0; i < 3; i++) {
        int ret_thread = pthread_create(&thread_id[i], NULL, talk, (void*)(intptr_t)ss[i]);
        // printf("i%d %d\n", i, ret_thread);
    }
    }

    pthread_exit(NULL);

    return 0;
}
