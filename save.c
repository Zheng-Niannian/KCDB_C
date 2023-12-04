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
    ULL kv[100000];
    int s_size[100000];
    int num[100000];
    int recycle[100000], top;
    int tot;
}T;

void node_init(int x){
    int i;
    for(i = 0; i < 16; i ++) T.s[x][i] = 0;
    T.value[x] = 0;
    T.flag[x] = false;
    T.kv[x] = 0;
    T.s_size[x] = 0;
    T.num[x] = 0;
}

void tree_init(){
        T.tot = 0;
        T.top = 0;
        int i;
        for(i = 0; i < 16; i ++) T.s[0][i] = 0;
        memset(T.value, 0, sizeof(T.value));
        memset(T.s_size, 0, sizeof(T.s_size));
        memset(T.num, 0, sizeof(T.num));
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



void merge(int now){
    if(T.flag[now]){
        printf("???\n");
        return ;
    }
    if(T.s_size[now] != 1){
        printf("???\n");
        return ;
    }
    int i, go = 0;
    for(i = 0; i <= 15; i ++){
        if(T.s[now][i]){
            go = T.s[now][i];
            T.s[now][i] = 0;//!
            T.s_size[now] = 0;//!
            break;
        }
    }
    T.kv[now] = (T.kv[now] << (4 * T.num[go])) + T.kv[go];
    // printf("now %d kv after %llu go = %d\n", now, T.kv[now], go);
    if(T.s_size[go]){
        for(i = 0; i <= 15; i ++){
            T.s[now][i] = T.s[go][i];
        }
        T.s_size[now] = T.s_size[go];
    }
    if(T.flag[go]){
        
        T.flag[go] = false;

    }
    T.num[now] += T.num[go];
    T.recycle[++ T.top] = go;
}

void split(int now, int pos){

    int go = T.top >= 1 ? T.recycle[T.top --] : ++ T.tot;
    // printf("-------------------split now %d pos %d go %d\n", now, pos, go);
    ULL all = T.kv[now], allnum = T.num[now];
    T.kv[now] = all / (1 << ((allnum - pos) * 4)), T.num[now] = pos;
    T.kv[go] = all % (1 << ((allnum - pos) * 4)), T.num[go] = allnum - pos;

    T.s_size[go] = T.s_size[now];
    int i;
    for(i = 0; i < 16; i ++) T.s[go][i] = T.s[now][i];
    T.s_size[now] = 1;
    for(i = 0; i < 16; i ++) T.s[now][i] = 0;
    // printf("!!!!!!!!!!! goto %d\n", T.kv[go] / (1 << ((T.num[go] - 1) * 4)));
    T.s[now][T.kv[go] / (1 << ((T.num[go] - 1) * 4))] = go;

    if(T.flag[now]) {
        T.flag[go] = true;
        T.value[go] = T.value[now];

        T.flag[now] = false;
        T.value[now] = 0;
    }
    
}

int match(ULL a, int lena, ULL b, int lenb){//匹配2个01串，以4为单位，看能匹配到第几个块， len为块数
    int i = (lena - 1) * 4, j = (lenb - 1) * 4;
    int ans = 0;
    for(; i >= 0 && j >= 0; i -= 4, j -= 4){
        int x = a / (1ll << i);
        int y = b / (1ll << j);
        if(x != y) break;
        ans ++;
        
    }
    return ans;
}

void Tinsert(ULL k, ULL v){
    // printf("--------------------insert\n");
    int now = 0; ULL rest = k;
    int i;
    for(i = 60; i >= 0; i -= 4){
        int x = rest / (1ll << i);
        // printf("----> i %lld x %d rest %llu\n", (1ll << i), x, rest % (1ll << i));
        // printf("???now %d go %d num[go] %d\n", now, T.s[now][x], T.num[T.s[now][x]]);
        if(T.num[T.s[now][x]] > 1){
            
            int num = match(rest, (i / 4) + 1, T.kv[T.s[now][x]], T.num[T.s[now][x]]);
            i -= 4 * (num - 1);
            if(num == T.num[T.s[now][x]]){
                now = T.s[now][x];
            } else {
                split(T.s[now][x], num);
                now = T.s[now][x];
            }
        } else {
            if(T.s[now][x]) now = T.s[now][x];
            else {
                if(T.top) {
                    T.s[now][x] = T.recycle[T.top --];
                    node_init(T.s[now][x]);
                } else T.s[now][x] = ++ T.tot;
                // printf("create new node %d\n", T.tot);
                T.s_size[now] ++;
                // int j = 0;
                // for(j = 0; j < 16; j ++) T.s[T.s[now][x]][j] = 0;
                T.kv[T.s[now][x]] = x;
                T.num[T.s[now][x]] = 1;
                // printf("----> s_size[now] %d\n", T.s_size[now]);
                if(T.s_size[now] == 1 && !T.value[now] && now != 0){
                    merge(now);
                } else now = T.s[now][x];
            }
            
        }
        rest %= (1ll << i);
    }
    if(T.flag[now]) {
        printf(">k = %llu is exist, the value has been overwritten! %d\n", k, now);
    }
    // printf("!!!%llu\n", T.kv[now]);
    T.value[now] = v, T.flag[now] = true;
}

struct msg Tfind(ULL k){
    int now = 0; ULL rest = k;
    int i;
    // printf("-----------------find\n");//
    for(i = 60; i >= 0; i -= 4){
        int x = rest / (1ll << i);
        // printf("----> i %lld x %d rest %llu\n", (1ll << i), x, rest % (1ll << i));
        // printf("???now %d go %d num[go] %d\n", now, T.s[now][x], T.num[T.s[now][x]]);
        if(T.num[T.s[now][x]] > 1){
            int num = match(rest, (i / 4) + 1, T.kv[T.s[now][x]], T.num[T.s[now][x]]);
            i -= 4 * (num - 1);
            // printf("match %d\n", num);
            if(num == T.num[T.s[now][x]]){
                now = T.s[now][x];
            } else {
                printf(">k = %llu is absent !!!\n", k);
                return (struct msg){0, false};
            }
        }
        else if(T.s[now][x]) now = T.s[now][x];
        // printf("-----> %d\n", now);
        rest %= (1ll << i);
    }
    if(!T.flag[now]) {
        printf(">k = %llu is absent\n", k);
        return (struct msg){0, false};
    }
    return (struct msg){T.value[now], true};       
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
                //printf("input 'savefile filename' to save data into file\n");
                //printf("input 'load filename' to load data from file\n");
                //printf("length of 'filename' must < 100\n");
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
           // case 3:
           //     scanf("%s", name);
           //     Tsave(name);
           //     break;
           // case 4:
           //     scanf("%s", name);
           //     Tload(name);
           //    break;            
            default:
                printf(">unknow operation\n");
                break;
        }
    }
    return 0;
}
