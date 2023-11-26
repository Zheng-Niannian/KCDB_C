#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

const char help_[] = {"help"};
const char save_[] = {"save"};
const char find_[] = {"find"};
const char savefile_[] = {"savefile"};
const char load_[] = {"load"};


bool check(const char *a, const char *b){
    int len_a = strlen(a);
    int len_b = strlen(b);
    if(len_a != len_b) return false;
    int i;
    for(i = 0; i < len_a; i ++){
        if(a[i] != b[i]) return false;
    }
    return true;
}

int get_type(char *s){
    if(check(help_, s)) return 0;
    else if(check(save_, s)) return 1;
    else if(check(find_, s)) return 2;
    else if(check(savefile_, s)) return 3;
    else if(check(load_, s)) return 4;
    return -1;
}


#define ULL unsigned long long 

struct msg{
    ULL v;
    bool flag;
};

struct Tree{
    int s[100000][16];//4bit  0 ~ 2^4 - 1
    ULL value[100000]; bool flag[100000];
    int tot;
}T;

void tree_init(){
        T.tot = 0;
        int i;
        for(i = 0; i < 16; i ++) T.s[0][i] = 0;
        memset(T.value, 0, sizeof(T.value));
    }


void Tsave(char *name){
    FILE* f = fopen(name, "w");
    fprintf(f, "%d\n", T.tot);
    int i, j;
    for(i = 0 ; i <= T.tot; i ++){
        int have = 0;
        for(j = 0; j <= 15; j ++){
            if(T.s[i][j]) have ++;
        }
        fprintf(f, "%d ", have);
        for(j = 0; j <= 15; j ++){
            if(T.s[i][j]) fprintf(f, "%d %d ", j, T.s[i][j]);    
        }
        fprintf(f, "%d ", T.flag[i]);
        if(T.flag[i]) fprintf(f, "%llu\n", T.value[i]);
        else fprintf(f, "\n");
    }
    fclose(f);
}

void Tload(char *name){
    FILE* f = fopen(name, "r");
    fscanf(f, "%d", &(T.tot));
    printf("load... %d\n", T.tot);
    int i, j;
    for(i = 0; i <= T.tot; i ++){
        int have;
        fscanf(f, "%d", &have);
        for(j = 0; j < have; j ++){
            int id, p;
            fscanf(f, "%d%d", &id, &p);
            T.s[i][id] = p;
        }
        
        int z = 0;
        fscanf(f, "%d", &z);
        if(z){
            ULL v;
            fscanf(f, "%llu", &v);
            T.value[i] = v;
            T.flag[i] = true;
            // printf("---- %d %d %llu\n", i, z, v);
        }
        else T.flag[i] = false;
    }
    fclose(f);
}

void Tinsert(ULL k, ULL v){
    int now = 0; ULL rest = k;
    int i;
    for(i = 60; i >= 0; i -= 4){
        int x = rest / (1ll << i);
        rest %= (1ll << i);
        // printf("----> i %lld x %d rest %llu\n", (1ll << i), x, rest);
        if(T.s[now][x]) now = T.s[now][x];
        else {
            T.s[now][x] = ++ T.tot;
            now = T.tot;
            int j = 0;
            for(j = 0; j < 16; j ++) T.s[now][j] = 0;
        }
    }
    if(T.flag[now]) {
        printf(">k = %llu is exist, the value has been overwritten! %d\n", k, now);
    }
    T.value[now] = v, T.flag[now] = true;
}

struct msg Tfind(ULL k){
    int now = 0; ULL rest = k;
    int i;
    for(i = 60; i >= 0; i -= 4){
        int x = rest / (1ll << i);
        rest %= (1ll << i);
        if(T.s[now][x]) now = T.s[now][x];
        // printf("-----> %d\n", now);
    }
    if(!T.flag[now]) {
        printf(">k = %llu is absent\n", k);
        return (struct msg){0, false};
    }
    return (struct msg){T.value[now], true};       
}

void split(){//radix tree

}

void compress(){

}


int main(){
    while(1){
        printf("input help to get info ...\n");
        char s[100];
        scanf("%s", s);
        ULL k, v;                
        char name[100];
        switch (get_type(s))
        {
            case 0:
                printf("------------------------------------------------------\n");
                printf("input 'save k v' to save\n");
                printf("input 'find k' to find v by k\n");
                printf("input 'savefile filename' to save data into file\n");
                printf("input 'load filename' to load data from file\n");
                printf("length of 'filename' must < 100\n");
                printf("------------------------------------------------------\n");
                break;

            case 1:

                scanf("%llu%llu", &k, &v);
                Tinsert(k, v);
                break;
            
            case 2:
                scanf("%llu", &k);
                struct msg res = Tfind(k);
                if(res.flag) {
                    printf(">the value is %llu\n", res.v);
                }
                break;
            case 3:
                scanf("%s", name);
                Tsave(name);
                break;
            case 4:
                scanf("%s", name);
                Tload(name);
                break;            
            default:
                printf(">unknow operation\n");
                break;
        }
    }
    return 0;
}
