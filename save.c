#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define maxn 400000
const char op_[][11] = {{"help"}, {"save"}, {"find"}, {"savefile"}, {"load"}, {"find_less"}, {"find_more"},{"update"}, {"delete"}};

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
    //int father;
};

struct Node * new_node(int type);
struct Node4{
    int tot;
    int key[4];//значение к
    struct Node* son[4];//Значение дочернего узла в соответствующей позиции
    // int type[4];
};
void node4_init(struct Node4 *p){//инициализация
    p->tot = 0;
    int i;
    for(i = 0; i < 4; i ++) p->key[i] = -1;
}
void node4_insert(struct Node4 *p, int key, struct Node * pos){//Вставьте существующий узел или вставьте новый узел
    if(p->tot >= 4){
        printf("already full\n");
        return ;
    }
    (p->tot) ++;
    int i;
    for(i = 0; i < 4; i ++){
        if(p->key[i] != -1) continue;
        // printf("create new node on %d\n", i);
        p->key[i] = key;
        if(pos == NULL) p->son[i] = new_node(1);
        else p->son[i] = pos;
        break;
    }
}

struct Node16{
    int tot;
    int key[16];
    struct Node* son[16];
    // int type[16];
};
void node16_init(struct Node16 *p){
    p->tot = 0;
    int i;
    for(i = 0; i < 16; i ++) p->key[i] = -1;
}
void node16_insert(struct Node16 *p, int key, struct Node * pos){
    if(p->tot >= 16){
        printf("already full\n");
        return ;
    }
    // p->key[p->tot] = key;
    // if(pos == NULL){
    //     p->son[p->tot] = new_node(1);
    // } else {
    //     p->son[p->tot] = pos;
    // }
    (p->tot) ++;
    int i;
    for(i = 0; i < 16; i ++){
        if(p->key[i] != -1) continue;
        p->key[i] = key;
        if(pos == NULL) p->son[i] = new_node(1);
        else p->son[i] = pos;
        break;
    }
}

struct Node48{
    int tot;
    char key[256];//k->son
    struct Node * son[48];
    // int type[48];
};
void node48_init(struct Node48 *p){
    p->tot = 0;
    int i;
    for(i = 0; i < 256; i ++) {
        p->key[i] = -1;
        p->son[i] = NULL;
    }
}
void node48_insert(struct Node48 *p, int key, struct Node * pos){
    if(p->key[key] != -1){
        printf("already exist\n");
        return ;
    }
    if(p->tot >= 48){
        printf("already full\n");
        return ;
    }
    // p->key[key] = p->tot;
    // if(pos == NULL){
    //     p->son[p->tot] = new_node(1);
    // } else {
    //     p->son[p->tot] = pos;
    // }
    (p->tot) ++;
    int i;
    for(i = 0; i < 48; i ++){
        if(p->son[i] != NULL) continue;
        p->key[key] = i;
        if(pos == NULL) {
            p->son[i] = new_node(1);
        }
        else {
            p->son[i] = pos;
        } 
        break;
    }
}

struct Node256{
    struct Node * son[256];
    int tot;
    // int type[256];
};
void node256_init(struct Node256 *p){
    int i;
    p->tot = 0;
    for(i = 0; i < 256; i ++) p->son[i] = NULL;
}
void node256_insert(struct Node256 *p, int key, struct Node * pos){
    if(p->son[key] != NULL){
        printf("already exist\n");
        return ;
    }
    (p->tot) ++;
    if(pos == NULL){
        p->son[key] = new_node(1);
    } else {
        p->son[key] = pos;
    }
}

struct Node{
    struct Node4 * node4;
    struct Node16 * node16;
    struct Node48 * node48;
    struct Node256 * node256;
    ULL val;
    int type;
    short end;//Является ли метка конечным узлом
    struct Node * end_pos;//Укажите на узел сохраненного значения
};


void node_init(struct Node *p, int type){//Инициализировать узел
    p->type = type;
    // printf("????%d\n", p->type);
    p->end = 0;
    p->end_pos = NULL;
    p->node4 = NULL, p->node16 = NULL, p->node48 = NULL, p->node256 = NULL;
    switch (type)
    {
    case 1:
        p->node4 = (struct Node4 *) malloc(sizeof(struct Node4));
        node4_init(p->node4);
        break;
    case 2:
        p->node16 = (struct Node16 *) malloc(sizeof(struct Node16));
        node16_init(p->node16);
        break;
    case 3 :
        p->node48 = (struct Node48 *) malloc(sizeof(struct Node48));
        node48_init(p->node48);
        break;
    case 4:
        p->node256 = (struct Node256 *) malloc(sizeof(struct Node256));
        node256_init(p->node256);
        break;
    case 5://Это узел хранения ценностей
        p->val = 0;
        break;
    default:
        printf(">err: type not exist!\n");
        break;
    }
}

void node_free(struct Node *p){//освободить узел
    switch (p->type)
    {
    case 1:
        free(p->node4);
        break;
    case 2:
        free(p->node16);
        break;
    case 3 :
        free(p->node48);
        break;
    case 4:
        free(p->node256);
        break;
    }
}

struct Node * new_node(int type){
    struct Node * p = (struct Node *)malloc(sizeof(struct Node));
    node_init(p, type);
    return p;
}

int node_size(struct Node *p){//Запрос количества дочерних узлов узла
    switch (p->type)
    {
    case 1:
        return p->node4->tot;
        break;
    case 2:
        return p->node16->tot;
    case 3:
        return p->node48->tot;
    case 4:
        return p->node256->tot;
    case 5:
        return 1;
    default:
        return 0;
        break;
    }
}

struct Node *root;

void tree_init(){//Инициализировать дерево
    root = new_node(1);
    // printf("!!!!!!%d\n",root->type);
}


int match(ULL a, int lena, ULL b, int lenb){ 
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

void expansion(struct Node * x){//Узел расширения
    if(x->type >= 4){
        printf("wrong type !\n");
        return ;
    }
    int i;
    switch (x->type)
    {
    case 1://node4->16
        x->type = 2;
        x->node16 = (struct Node16 *)malloc(sizeof(struct Node16));
        node16_init(x->node16);
        // struct Node16 *p = x->node16;
        // p->tot = x->node4->tot;
        for(i = 0; i < 4; i ++){
            if(x->node4->key[i] == -1) continue;
            node16_insert(x->node16, x->node4->key[i], x->node4->son[i]);
        }
        free(x->node4);
        break;
    case 2://node16->48
        x->type = 3;
        x->node48 = (struct Node48 *)malloc(sizeof(struct Node48));
        node48_init(x->node48);
        for(i = 0; i < 16; i ++){
            if(x->node16->key[i] == -1) continue;
            node48_insert(x->node48, x->node16->key[i], x->node16->son[i]);
        }
        free(x->node16);
        break;
    case 3://node48->256
        x->type = 4;
        x->node256 = (struct Node256 *)malloc(sizeof(struct Node256));
        node256_init(x->node256);
        for(i = 0; i < 256; i ++){
            if(x->node48->key[i] == -1) continue;
            node256_insert(x->node256, i, x->node48->son[x->node48->key[i]]);
        }
        free(x->node48);
    default:
        break;
    }
}

void reduce(struct Node *x){//Сжать узел

}

void Tinsert(ULL k, ULL v, int flag){//вставлять
    // printf("--------------------insert\n");
    struct Node * now = root; ULL rest = k;
    int i;
    for(i = 56; i >= 0; i -= 8){//Принимая 8 бит за единицу
        int x = rest / (1ll << i);
        // printf("--->%d %d %llu now: %p\n", i, x, rest, now);
        int j;int done = 0;
        // printf("---- %d\n", now->type);
        switch (now->type)
        {
        case 1://node4
            // struct Node4 *p = now->node4;
            for(j = 0; j < 4; j ++){
                if(now->node4->key[j] == -1) continue;
                if(now->node4->key[j] == x) {
                    // printf("!!!!!\n");
                    now = now->node4->son[j];
                    done = 1;
                    break; 
                }
            }
            if(done == 0){
                if(now->node4->tot == 4){//Узел заполнен
                    expansion(now);//Узел расширения
                    // printf("----");
                    node16_insert(now->node16, x, NULL);
                    for(j = 0; j < 16; j ++){
                        if(now->node16->key[j] == -1) continue;
                        // printf("!!!%d %d\n", j, now->node16->key[j]);
                        if(now->node16->key[j] == x) {
                            // printf("???%d\n", j);
                            now = now->node16->son[j];
                            done = 1;
                            break; 
                        }
                    }
                } else {
                    node4_insert(now->node4, x, NULL);
                    for(j = 0; j < 4; j ++){
                        if(now->node4->key[j] == -1) continue;
                        if(now->node4->key[j] == x) {
                            now = now->node4->son[j];
                            // printf("^^^^^^\n");
                            done = 1;
                            break; 
                        }
                    }
                }
                
            }        
            break;
        case 2://node16
            // struct Node16 *p = now->node16;
            for(j = 0; j < now->node16->tot; j ++){
                if(now->node16->key[j] == -1) continue;
                if(now->node16->key[j] == x) {
                    now = now->node16->son[j];
                    done = 1;
                    break; 
                }
            }
            if(done == 0){
                if(now->node16->tot == 16){
                    expansion(now);
                    node48_insert(now->node48, x, NULL);
                    now = now->node48->son[(now->node48->key[x])];
                } else {
                    node16_insert(now->node16, x, NULL);
                    for(j = 0; j < 16; j ++){
                        if(now->node16->key[j] == -1) continue;
                        if(now->node16->key[j] == x) {
                            now = now->node16->son[j];
                            done = 1;
                            break; 
                        }
                    }
                }
                
            }   
            break;
        case 3://node48
            if(now->node48->key[x] == -1){
                if(now->node48->tot >= 48){
                    expansion(now);
                    node256_insert(now->node256, x, NULL);
                    now = now->node256->son[x];
                } else {
                    node48_insert(now->node48, x, NULL);
                    now = now->node48->son[x];
                }
            } else now = now->node48->son[x];
            break;
        case 4://node256
            if(now->node256->son[x] == NULL) {
                node256_insert(now->node256, x, NULL);
            }
            now = now->node256->son[x];
            break;
        default:
            break;
        }
        rest %= (1ll << i);
    }
    // printf("now = %p type = %d\n", now, now->type);
    if(now->end == 1) {//Узел сохраненного значения
        if(flag == 0) printf(">k = %llu is exist, do you mean to update the value %llu?", k, now->end_pos->val);// the value has been overwritten!\n", k);
        else {
            now = now->end_pos;
            now->val = v;
            printf(">k = %llu is exist, the value has been overwritten!\n", k);
        }
    }
    else {//create new val node
        if(flag == 0){
            now->end = 1;
            now->end_pos = new_node(5);
            now = now->end_pos;
            now->val = v;
        } else {
            printf(">k - %llu is absent?\n", k);
        }
        
    }
}

struct msg Tfind(ULL k){
    struct Node * now = root; ULL rest = k;int last = 0;
    int i;
    // printf("-----------------find\n");//
    for(i = 56; i >= 0; i -= 8){
        int x = rest / (1ll << i);
        int err = 0;
        int j;int done = 0;
        switch (now->type)
        {
        case 1://node4
            // struct Node4 *p = now->node4;
            for(j = 0; j < 4; j ++){
                if(now->node4->key[j] == -1) continue;
                if(now->node4->key[j] == x) {
                    now = now->node4->son[j];
                    done = 1;
                    break; 
                }
            }
            if(done == 0){
                err = 1;
            }        
            break;
        case 2://node16
            // struct Node16 *p = now->node16;
            for(j = 0; j < 16; j ++){
                if(now->node16->key[j] == -1) continue;
                if(now->node16->key[j] == x) {
                    now = now->node16->son[j];
                    done = 1;
                    break; 
                }
            }
            if(done == 0){
                err = 1;
            }   
            break;
        case 3://node48
            if(now->node48->key[x] == -1){
                err = 1;
            } else now = now->node48->son[x];
            break;
        case 4://node256
            if(now->node256->son[x] == NULL) {
                err = 1;
            }
            now = now->node256->son[x];
            break;
        }
        if(err == 1){
            printf(">k = %llu is absent\n", k);
            return (struct msg){0, false};
        }
        rest %= (1ll << i);
    }
    if(now->end == 0) {
        printf(">k = %llu is absent\n", k);
        return (struct msg){0, false};
    }
    now = now->end_pos;
    return (struct msg){now->val, true};       
}

bool Tupdate(ULL k, ULL v){//Изменить значение v
    struct msg res = Tfind(k);
    if(res.flag == false){//к не существует
        //printf(">k = %llu is absent\n", k);//Это сообщение было выведено раньше
        return false;
    } else Tinsert(k, v, 1);
    return true;
}

struct Node * last[10000];
int way[10000];
int top;
bool Tdelete(ULL k){
    struct msg res = Tfind(k);
    if(res.flag == false){//к не существует
        printf(">k = %llu is absent\n", k);
        return false;
    } 
    SDelete(res.v);

    struct Node * now = root; ULL rest = k;
    top = 0;
    int i;
    // printf("-----------------find\n");//
    for(i = 56; i >= 0; i -= 8){
        int x = rest / (1ll << i);
        int err = 0;
        int j;int done = 0;
        switch (now->type)
        {
        case 1://node4
            // struct Node4 *p = now->node4;
            for(j = 0; j < 4; j ++){
                if(now->node4->key[j] == -1) continue;
                if(now->node4->key[j] == x) {
                    now = now->node4->son[j];
                    done = 1;
                    break; 
                }
            }
            if(done == 0){
                err = 1;
            }        
            break;
        case 2://node16
            // struct Node16 *p = now->node16;
            for(j = 0; j < 16; j ++){
                if(now->node16->key[j] == -1) continue;
                if(now->node16->key[j] == x) {
                    now = now->node16->son[j];
                    done = 1;
                    break; 
                }
            }
            if(done == 0){
                err = 1;
            }   
            break;
        case 3://node48
            if(now->node48->key[x] == -1){
                err = 1;
            } else now = now->node48->son[x];
            break;
        case 4://node256
            if(now->node256->son[x] == NULL) {
                err = 1;
            }
            now = now->node256->son[x];
            break;
        }
        last[++ top] = now;//record
        way[top] = x;
        if(err == 1){
            printf(">k = %llu is absent\n", k);
            return false;
        }
        rest %= (1ll << i);
    }
    if(now->end == 0) {
        printf(">k = %llu is absent\n", k);
        return false;
    }
    now->end = 0;
    free(now->end_pos);
    last[0] = root;
    while(top){
        // printf("way is %d pos is %p\n", way[top], last[top]);
        if(last[top]->end == 1) break;
        if(node_size(last[top]) >= 1) break;
        struct Node * now = last[top - 1];
        switch (now->type)
        {
        case 1:
            for(i = 0 ; i < 4; i ++){
                if(now->node4->key[i] == -1) continue;
                if(now->node4->key[i] == way[top]){
                    now->node4->key[i] = -1;
                    now->node4->son[i] = NULL;
                    now->node4->tot --;
                    break;
                }
            }
            break;
        case 2:
            for(i = 0 ; i < 16; i ++){
                if(now->node16->key[i] == -1) continue;
                if(now->node16->key[i] == way[top]){
                    now->node16->key[i] = -1;
                    now->node16->son[i] = NULL;
                    now->node16->tot --;
                    if(now->node16->tot <= 2) reduce(now);
                    break;
                }
            }
            break;
        case 3:
            now->node48->son[now->node48->key[way[top]]] = NULL;
            now->node48->key[way[top]] = -1;
            now->node48->tot --;
            if(now->node48->tot <= 14) reduce(now);
            break;
        case 4:
            now->node256->son[way[top]] = NULL;
            now->node256->tot --;
            if(now->node256->tot <= 40) reduce(now);
            break;
        default:
            break;
        }
        free(last[top]);
        top --;
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
                Tinsert(k, v, 0);
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
