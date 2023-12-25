#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

//const char help_[] = {"help"};//Сохранить тип команды
//const char save_[] = {"save"};
//const char find_[] = {"find"};
//const char savefile_[] = {"savefile"};
//const char load_[] = {"load"};
// const char find_less_[] = {"find_less"};
// const char find_more_[] = {"find_more"};
const char op_[][10] = {{"help"}, {"save"}, {"find"}, {"savefile"}, {"load"}, {"find_less"}, {"find_more"}};

//Сравните две строки на равенство
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

//оптимизация
int get_type(char *s){
    // printf("!!!%s\n", s);
    int len = 9, i;
    for(i = 0; i < len; i ++){
        if(check(op_[i], s)) return i;
    }
    // printf("???");
    return -1;
}

#define ULL unsigned long long 
void SDelete(ULL x);
struct msg{
    ULL v;
    bool flag;
    int father;
};

#define maxn 100000
struct Tree{//trie ---> radix tree ---> Adaptive
    int s[maxn][16];//4bit  0 ~ 2^4 - 1
    ULL value[maxn]; bool flag[maxn];
    ULL kv[maxn];//Сохраняем значение текущего узла (без предков)
    int s_size[maxn];//Количество детей узла
    int num[maxn];//Количество реальных узлов, содержащихся в текущем узле
    int recycle[maxn], top;//Пул для восстановления, используемый для сжатия и разделения узлов
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


void Tsave(char *name){//Проблемы с хранением файлов рассматриваются здесь Структура хранения файлов нуждается в перепроектировании и в данный момент недоступна.
    FILE* f = fopen(name, "w");//перезаписать
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

void Tload(char *name){//Загрузка сохраненных данных
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
    if(T.flag[now]){// Текущий узел не может быть завершающим узлом
        printf("???\n");
        return ;
    }
    if(T.s_size[now] != 1){
        printf("???\n");
        return ;
    }
    int i, go = 0;
    for(i = 0; i <= 15; i ++){// найдите дочерние узлы для сжатия
        if(T.s[now][i]){
            go = T.s[now][i];
            T.s[now][i] = 0;//!
            T.s_size[now] = 0;//!
            break;
        }
    }
    T.kv[now] = (T.kv[now] << (4 * T.num[go])) + T.kv[go];// Префиксная консолидация
    // printf("now %d kv after %llu go = %d\n", now, T.kv[now], go);
    if(T.s_size[go]){// Если у дочернего узла еще есть дети
        for(i = 0; i <= 15; i ++){// напрямую перезаписываем дочерние узлы в текущий узел
            T.s[now][i] = T.s[go][i];
        }
        T.s_size[now] = T.s_size[go];
    }
    if(T.flag[go]){// Если объединенный узел является терминирующим узлом
        
        T.flag[go] = false;

    }
    T.num[now] += T.num[go];
    T.recycle[++ T.top] = go;//Поместить в пул переработки
}
void split(int now, int pos){
    //radix tree требует разделения и сжатия узлов, 
    //now — точка, которую нужно разделить, pos — это позиция, которую нужно разделить.
    int go = T.top >= 1 ? T.recycle[T.top --] : ++ T.tot;//Сначала используйте переработанные узлы, если они есть.
    // printf("-------------------split now %d pos %d go %d\n", now, pos, go);
    ULL all = T.kv[now], allnum = T.num[now];
    T.kv[now] = all / (1 << ((allnum - pos) * 4)), T.num[now] = pos;
    T.kv[go] = all % (1 << ((allnum - pos) * 4)), T.num[go] = allnum - pos;

    T.s_size[go] = T.s_size[now];// Сначала передайте новому узлу исходное количество детей
    int i;
    for(i = 0; i < 16; i ++) T.s[go][i] = T.s[now][i];
    T.s_size[now] = 1;//измените количество дочерних узлов исходного узла еще раз
    for(i = 0; i < 16; i ++) T.s[now][i] = 0;
    // printf("!!!!!!!!!!! goto %d\n", T.kv[go] / (1 << ((T.num[go] - 1) * 4)));
    T.s[now][T.kv[go] / (1 << ((T.num[go] - 1) * 4))] = go;// Возьмите только первый блок

    if(T.flag[now]) {
        T.flag[go] = true;
        T.value[go] = T.value[now];

        T.flag[now] = false;
        T.value[now] = 0;
    }
    
}

int match(ULL a, int lena, ULL b, int lenb){//Сопоставьте 2 строки 01 в количестве 4 штук, чтобы узнать, сколько блоков может совпасть, len - количество блоков.
    int i = (lena - 1) * 4, j = (lenb - 1) * 4;
    int ans = 0;
    for(; i >= 0 && j >= 0; i -= 4, j -= 4){
        int x = a / (1ll << i);
        int y = b / (1ll << j);
        if(x != y) break;
        ans ++;
        
    }
    return ans;//Количество совпавших блоков
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
        else if(T.s[now][x]) {
            last = now;
            now = T.s[now][x];
        } else {
            if(!T.flag[now]) {
                printf(">k = %llu is absent\n", k);
                return (struct msg){0, false, 0};
            }
        }
        // printf("-----> %d\n", now);
        rest %= (1ll << i);
    }
    if(!T.flag[now]) {
      printf(">k = %llu is absent\n", k);
      if(T.s_size[now] == 0){
         if(last != 0){
            T.s_size[last] --;
            for(i = 0; i < 16; i ++){
                if(T.s[last][i] == now) {
                    T.s[last][i] = 0;
                }
            }
        } 
        T.recycle[++ T.top] = now;
    }
    return (struct msg){0, false, 0};
}
 return (struct msg){now, true, last};       
}

bool Tupdate(ULL k, ULL v){//Измените значение v
    struct msg res = Tfind(k);
    if(res.flag == false){//k не существует

        return false;
    }
    T.value[res.v] = v;
    return true;
}

bool Tdelete(ULL k){
    struct msg res = Tfind(k);
    if(res.flag == false){//k не существует
        printf(">k = %llu is absent\n", k);
        return false;
    } 
    T.recycle[++ T.top] = res.v;
    SDelete(T.value[res.v]);
   
    int now = res.father;
    if(T.s_size[res.father] > 0) T.s_size[res.father] --;
    int i;
    for(i = 0; i < 16; i ++) {
        if(T.s[res.father][i] == res.v) {
            T.s[res.father][i] = 0;
            break;
        }
    }
   
    
}

//Структуры данных, отвечающие за поиск
int Sroot, Sn, Stot;
int Ssize[maxn], Sf[maxn], Scnt[maxn], Sson[maxn][2];
ULL Sdate[maxn];
//size - это сумма cnt поддеревьев, в которых находится узел
void Supdate(int x)
{
    Ssize[x]=Ssize[Sson[x][0]]+Ssize[Sson[x][1]]+Scnt[x];
}
void Srotate(int x)//вращать
{
    int y = Sf[x], z = Sf[y], k = Sson[y][1]==x;
    Sson[z][Sson[z][1] == y] = x, Sf[x] = z;
    Sson[y][k] = Sson[x][k^1];
    Sf[Sson[x][k^1]] = y;
    Sson[x][k^1] = y;
    Sf[y] = x;
    Supdate(y), Supdate(x);
}
void Ssplay(int x, int goal)//splay
{
    while(Sf[x] != goal)
    {
        int y = Sf[x], z = Sf[y];
        if(z != goal)
            (Sson[y][0] == x) ^ (Sson[z][0] == y) ? Srotate(x): Srotate(y);
        Srotate(x);
    }
    if(!goal) Sroot = x;
}
void SInsert(ULL x)//Вставить узел
{
    int now = Sroot,fa=0;
    while(now && Sdate[now] != x)
    {
        fa = now;
        now = Sson[now][x > Sdate[now]];
    }
    if(now)Scnt[now]++;
    else 
    {
        now = ++Stot;
        if(fa) Sson[fa][x > Sdate[fa]] = now;
        Sson[Stot][0] = Sson[Stot][1] = 0;
        Sf[Stot] = fa, Sdate[Stot] = x;
        Scnt[Stot] = Ssize[Stot] = 1;
    }
    Ssplay(now,0);
}

void Sfind(ULL x)
{
    int now = Sroot;
    if(!now)return ;
    while(Sson[now][x > Sdate[now]] && x != Sdate[now]){
        now = Sson[now][x > Sdate[now]];
    }
    Ssplay(now,0);
}

int SNext(ULL x,int f)//Операция поиска
{
    Sfind(x);
    int now = Sroot;
    if((Sdate[now]>x && f) || (Sdate[now]<x && !f))return now;
    now = Sson[now][f];
    while(Sson[now][f^1]) {
        now = Sson[now][f^1];
    }
    return now;
}

void SDelete(ULL x)//Удаление по значению
{
    int last=SNext(x, 0);
    int next=SNext(x, 1);
//    printf("------%llu %llu\n", Sdate[last], Sdate[next]);
    if(last == 0){
        Ssplay(next, 0);
      
    } else if(next == 0){
        Ssplay(last, 0);
      
        int del = Sson[next][1];
        if(Scnt[del] > 1)
            Scnt[del] --, Ssplay(del, 0);
        else Sson[next][1] = 0;
        return ;
    } else Ssplay(last, 0); Ssplay(next, last);
    // printf("------%llu %llu\n", Sdate[last], Sdate[next]);
    int del = Sson[next][0];
    if(Scnt[del] > 1)
        Scnt[del] --, Ssplay(del, 0);
    else Sson[next][0] = 0;
}

bool KVUpdate(ULL k, ULL v){//Обновление пары «ключ-значение»
    struct msg res = Tfind(k);
    if(!res.flag) return false;
    ULL last = T.value[res.v];
    if(!Tupdate(k, v)) return false;
    // printf("tttttt--------\n");
    SDelete(last);
    // printf("????");
    SInsert(v);
    // printf("!!!!\n");
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
                printf("input 'update k v' to update the value on a pair of (k, v)");
                printf("input 'delete k' to delete by k\n");
                //printf("input 'savefile filename' to save data into file\n");
                //printf("input 'load filename' to load data from file\n");
                printf("input 'find_less k' to find the closest smaller value by v\n");
                printf("input 'find_more k' to find the closest bigger value by v\n");
                //printf("length of 'filename' must < 100\n");
                printf("------------------------------------------------------\n");
                break;

            case 1:

                scanf("%llu%llu", &k, &v);
                Tinsert(k, v);
                SInsert(k);
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
            case 5:
                scanf("%llu", &v);
                if(Ssize[Sroot] <= 0){
                    printf("data empty\n");
                } else {
                    int ans = SNext(v, 0);
                    // printf("---> ans %d\n", ans);
                    if(ans == 0) printf("no one is smaller than %llu\n", v);
                    else printf(">the value is %llu\n", Sdate[ans]);
                }
                break;
            case 6:
                scanf("%llu", &v);
                if(Ssize[Sroot] <= 0){
                    printf("data empty\n");
                } else {
                    int ans = SNext(v, 1);
                    if(ans == 0) printf("no one is smaller than %llu\n", v);
                    else printf(">the value is %llu\n", Sdate[ans]);
                }
                break;
            case 7:
                scanf("%llu%llu", &k, &v);
                KVUpdate(k, v);
                break;
            case 8:
                scanf("%llu", &k);
                Tdelete(k);
                break;           
            default:
                printf(">unknow operation\n");
                break;
        }
    }
    return 0;
}
