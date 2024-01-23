#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define maxn 400000
const char op_[][12] = {{"help"}, {"save"}, {"find"}, {"savefile"}, {"load"}, {"find_less"}, {"find_more"},{"update"}, {"delete"}};

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
    int len = 10, i;
    for(i = 0; i < len; i ++){    
        if(check(op_[i], s)) return i;
    }
    // printf("???");
    return -1;
}

#define ULL unsigned long long 
void SDelete(char * x);
struct msg{
    //ULL v;
    char *v
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
    char *val;
    int type;
    short end;//Является ли метка конечным узлом
    struct Node * end_pos;//Укажите на узел сохраненного значения

    struct Node * son;
    char *s;
    int have;//Длина строки внутри сжатого узла
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
        return ;
        break;
    }
    p->s = 0;
    p->have = 0;//node4~node256 могут стать узлами сжатия.
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


/*int match(ULL a, int lena, ULL b, int lenb){ 
    int i = (lena - 1) * 4, j = (lenb - 1) * 4;
    int ans = 0;
    for(; i >= 0 && j >= 0; i -= 4, j -= 4){
        int x = a / (1ll << i);
        int y = b / (1ll << j);
        if(x != y) break;
        ans ++;
        
    }
    return ans;
}*/

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
        if(x->type <= 1) {
         printf("wrong type !\n");
        return ;       
    }
    int i;
    switch (x->type)
    {
    case 2://node16-->node4
        x->type = 1;
        x->node4 = (struct Node4 *)malloc(sizeof(struct Node4));
        node4_init(x->node4);
        for(i = 0; i < 16; i ++){
            if(x->node16->key[i] == -1) continue;
            node4_insert(x->node4, x->node16->key[i], x->node16->son[i]);
        }
        free(x->node16);
        break;
    case 3://node48-->node16
        x->type = 2;
        x->node16 = (struct Node16 *)malloc(sizeof(struct Node16));
        node16_init(x->node16);
        for(i = 0; i < 256; i ++){
            if(x->node48->key[i] == -1) continue;
            node16_insert(x->node16, i, x->node48->son[x->node48->key[i]]);
        }
        free(x->node48);
        break;
    case 4://node256-->node48
        x->type = 3;
        x->node48 = (struct Node48 *)malloc(sizeof(struct Node48));
        node48_init(x->node48);
        for(i = 0; i < 256; i ++){
            if(x->node256->son[i] == NULL) continue;
            node48_insert(x->node48, i, x->node256->son[i]);
        }
        free(x->node256);
        break;
    default:
        break;
    }

}
bool match(struct Node * now, ULL *rest, int *pos, int type){
    //Попробуйте выполнить сопоставление внутри сжатого узла. Если совпадение не полностью, разделите сжатый узел и перейдите к новому узлу.
    // char *have = *rest;//При сжатии узлов для сопоставления необходимо сначала выполнить сопоставление внутри узла.
    int i;//type = 1 означает, что если обнаружено несоответствие или дочерний узел пуст, будет возвращено значение false, которое используется для функции find.
    char *ma = now->s; int tmp = now->have;
    /*for(i = *pos; i >= 0; i -= 8){
        if(tmp == 0) break;
        int x = have / (1ll << i);
        int y = ma / (1ll << ((tmp - 1) * 8));
        if(x != y) break;
        have %= (1ll << i);
        ma %= (1ll << ((tmp - 1) * 8));
        tmp --;
        
    }*/
    for(i = *pos; i < strlen(rest); i += 8){
        if(tmp == 0) break;
        int j; bool flag = true;
        for(j = 0; j < 8; j ++){
            if(rest[i + j] != ma[((now->have) - tmp) * 8 + j]){
                flag = false;
                break;
            }
        }
        if(flag == false) break;
        tmp --;
    }
    if(tmp == 0){//Точное совпадение внутри узла
        if(i >= strlen(rest)){//rest тоже точно совпадает
            if(type == 1){//rest совпало, но текущий узел не является узлом сохраненного значения, поэтому узел не может быть найден
                return false;
            }
            struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));//Создайте новый пустой узел
            newNode->type = now->type;
            newNode->node4 = now->node4;
            newNode->node16 = now->node16;
            newNode->node48 = now->node48;
            newNode->node256 = now->node256;
            newNode->have = 0;
            now->have --;
            //(now->s) >>= 8;
            int s_len = strlen(now->s);
            now->s[s_len - 8] = 0;
            now->type = 1;
            now->node4 = (struct Node4 *)malloc(sizeof(struct Node4));
            node4_init(now->node4);
            now->node16 = NULL;
            now->node48 = NULL;
            now->node256 = NULL;
            *rest %= (1ll << 8);
            //*pos = 0;
            *pos = strlen(rest) - 8;
            node4_insert(now->node4, *rest, newNode);
            return false;
        } else {
            //*rest = have;
            *pos = i;
            if(type == 1)  return true;
            return false;
        }
    } else {//Узел внутри не полностью совпадает
        if(i >= strlen(rest) && type == 1){//rest было сопоставлено, 
            //но текущий узел не является узлом сохраненного значения, поэтому ключ не может быть найден.
            return false;
        }
        struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));//Создайте новый узел сжатия
        newNode->type = now->type;
        newNode->node4 = now->node4;
        newNode->node16 = now->node16;
        newNode->node48 = now->node48;
        newNode->node256 = now->node256;//Новый узел напрямую наследует все дочерние узлы текущего узла.
        newNode->have = tmp - 1;
        newNode->val = NULL;
        newNode->s = NULL;
        //newNode->s = (now->s) % (1ll << ((tmp - 1) * 8)); 
        if(newNode->have > 0) {
             newNode->s = (char *)malloc((newNode->have) * 8 + 1);// Нужно дать еще один, чтобы поставить конечный символ
             newNode->s[newNode->have * 8] = 0;//конечный символ
             int rev = now->have - newNode->have;//Остальные сегменты старого узла (включая сегмент, который будет использоваться в качестве «ключевого»)
             rev *= 8;//Перемещаем в конец оставшегося сегмента
             int j;
             for(j = 0; j < newNode->have * 8; j ++){
                 newNode->s[j] = now->s[rev + j];
             }
        }
        // int key = ((now->s) / (1ll << ((tmp - 1) * 8))) % (1ll << 8);
        int key_t = (now->have - newNode->have - 1) * 8;
        int key = 0;
        int j;
        for(j = 0; j < 8; j ++) key = (key << 1) + now->s[key_t + j] - '0';// Получаем сегмент, который будет использоваться в качестве ключа
        now->type = 1;
        now->node4 = (struct Node4 *)malloc(sizeof(struct Node4));// Очистим оригинальные дочерние узлы текущего узла и присоединим новые
        node4_init(now->node4);
        now->node16 = NULL;
        now->node48 = NULL;
        now->node256 = NULL; 
        if(i >= strlen(rest)){//rest сопоставлен, и необходимо удалить промежуточный узел как узел хранения значений.
            struct Node * newMidNode = new_node(1);// Создаем новый пустой средний узел
            // int midkey = ((now->s) / (1ll << ((tmp) * 8))) % (1ll << 8);
            int midkey = 0;
            for(j = strlen(rest) - 8; j < strlen(rest); j ++) midkey = (midkey << 1) + rest[j] - '0';
            // (now->s) >>= 1ll << ((tmp + 1) * 8);

            now->s[(now->have - newNode->have - 1 - 1) * 8] = 0;//Обрежем сегмент, который является ключом и сегмент, который был выделен в качестве узла хранения значений
            now->have -= newNode->have + 1 + 1;
            // *rest %= (1ll << 8);//В rest оставим только последний сегмент
            // *pos = 0;//pos тоже оставим только последний сегмент
            *pos = strlen(rest) - 8;
            node4_insert(now->node4, midkey, newMidNode);// Присоединяем новый средний узел в качестве дочернего узла текущего узла       
            node4_insert(newMidNode->node4, key, newNode);//Присоединяем новый узел к новому среднему узлу
            return false;
        } 
        now->s[(now->have - newNode->have - 1) * 8] = 0;
        now->have -= newNode->have + 1;
        node4_insert(now->node4, key, newNode);
        // *rest = have;
        *pos = i;
        return false;
    }

void Tinsert(char * k, char * v, int flag){//вставлять
    struct Node * now = root; //ULL rest = k;
    char rest[100];
    memcpy(rest, k, sizeof(rest));
    int rest_len = strlen(rest);
    int i;
    int stay = 0;
    for(i = 0; i < rest_len; i += 8){//Принимая 8 бит за единицу      
        // printf("--->%d %d %llu now: %p\n", i, x, rest, now);
        int j;int done = 0;
        // printf("---- %d\n", now->type);
        if(now->have >= 1 && stay == 0){//Если текущий узел является сжатым узлом и вход в узел происходит впервые
            match(now, rest, &i, 0);
        }
        stay = 0;
        //int x = rest / (1ll << i);
        int x = 0;
        for(j = i; j < i + 8; j ++) x = (x << 1) + rest[j] - '0';
        switch (now->type)
        {
        case 1://node4
            for(j = 0; j < 4; j ++){
                if(now->node4->key[j] == -1) continue;
                if(now->node4->key[j] == x) {
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
                    if(now != root && now->node4->tot == 0 && now->end == 0 &&  i + 8 < rest_len){
                        //Если текущий узел не имеет дочерних элементов, не является корнем, не является узлом с хранимым значением и новый узел не будет узлом с сохраненным значением, то необходимо напрямую объединить новый узел с текущим узлом.
                        //Узлы сжатия не могут быть узлами хранения.
                       // now->s <<= 8;//Сжать новый узел в текущий узел
                       //now->s += x;
                        char * tmp;
                        if(now->s == NULL) {
                            tmp = (char *)malloc(9);
                        } else {
                            tmp = (char *)malloc(strlen(now->s) + 1 + 8);
                            memcpy(tmp, now->s, sizeof(now->s));
                        }
                        int tt = strlen(tmp);
                        int l;
                        for(l = tt + 7; l >= tt; l --) tmp[l] = x % 2, x >>= 1;
                        tmp[tt + 8] = 0;//\0
                        now->have ++;
                        stay = 1;
                        break;                      
                    } else{
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
       // rest %= (1ll << i);
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

struct msg Tfind(char *k){
    struct Node * now = root; char rest[100];int last = 0;
    int i;
    memcpy(rest, k, sizeof(rest));
    // printf("-----------------find\n");//
    for(i = 0; i < strlen(rest); i += 8){
        //printf("%d\n",i);
        int err = 0;
        int j;int done = 0;
        if(now->have >= 1){
            if(match(now, rest + i, &i, 1) == false) {
                printf(">k = %s is absent!\n", k);
                return (struct msg){NULL, false};
            }
        }
        //int x = rest / (1ll << i);
        int x = 0;
        for(j = i; j < i + 8; j ++) x = (x << 1) + rest[j] - '0';
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
            printf(">k = %s is absent\n", k);
            return (struct msg){NULL, false};
        }
        //rest %= (1ll << i);
    }
    if(now->end == 0) {
        printf(">k = %s is absent\n", k);
        return (struct msg){NULL, false};
    }
    now = now->end_pos;
    return (struct msg){now->val, true};       
}

bool Tupdate(char * k, char * v){//Изменить значение v
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
bool Tdelete(char *k){
    struct msg res = Tfind(k);
    if(res.flag == false){//к не существует
        printf(">k = %s is absent!\n", k);
        return false;
    } 
    //SDelete(res.v);

    struct Node * now = root;// ULL rest = k;
    char rest[100];
    memcpy(rest, k, sizeof(rest));
    top = 0;
    int i;
    // printf("-----------------find\n");//
    int rest_len = strlen(rest);
    for(i = 0; i < rest_len; i += 8){
        int err = 0;
        int j;int done = 0;
        if(now->have >= 1){
            if(match(now, rest + i, &i, 1) == false) {
                printf(">k = %s is absent!\n", k);
                return false;
            }
        }
        //int x = rest / (1ll << i);
        int x = 0;
        for(j = i; j < i + 8; j ++) x = (x << 1) + rest[j] - '0';
        
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
            printf(">k = %s is absent\n", k);
            return false;
        }
       //rest %= (1ll << i);
    }
    if(now->end == 0) {
        printf(">k = %s is absent\n", k);
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
char* Sdate[maxn];
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

int judge(char * x, char * y){//Сравните размер двух струн 01，"1"->"x > y", "0"->"x = y", "-1"->"x < y"
    int xx = strlen(x), yy = strlen(y);
    // int a = 0, b = 0;
    // while(a < xx && x[a] == '0') ++ a;
    // while(b < yy && y[b] == '0') ++ b;
    if(xx > yy) return 1;
    if(xx < yy) return -1;
    int i;
    for(i = 0; i < xx; i ++){
        if(x[i] < y[i]) return -1;
        if(x[i] > y[i]) return 1;
    }
    return 0;
}



void SInsert(ULL x)//Вставить узел
{
    int now = Sroot,fa=0;
    // while(now && Sdate[now] != x)
    while(now && judge(Sdate[now], x) != 0)
    {
        fa = now;
        // now = Sson[now][x > Sdate[now]];
        now = Sson[now][judge(x, Sdate[now]) >= 1];
    }
    if(now)Scnt[now]++;
    else 
    {
        // printf("create %d %d\n", now, fa);
        // printf("!!!!!%llu\n", Sson[0][0]);
        now = ++Stot;
        if(fa) Sson[fa][judge(x, Sdate[fa]) >= 1] = now;
        Sson[Stot][0] = Sson[Stot][1] = 0;
        Sf[Stot] = fa, Sdate[Stot] = x;
        Scnt[Stot] = Ssize[Stot] = 1;
        // printf("!!!!!%llu\n", Sson[0][0]);
    }
    Ssplay(now,0);
    // printf("!!!!!%llu\n", Sson[0][0]);
}

void Sfind(char * x)
{
    int now = Sroot;
    if(!now) return ;
    while(Sson[now][judge(x, Sdate[now]) >= 1] && judge(x, Sdate[now]) != 0){
        now = Sson[now][judge(x, Sdate[now]) >= 1];
    }
    Ssplay(now,0);
}

int SNext(char * x, int f)//Операция поиска
{
    Sfind(x);
    int now = Sroot;
    if((judge(Sdate[now], x) >= 1 && f) || (judge(Sdate[now], x) <= -1 && !f))return now;
    // printf("!!!%llu\n", Sdate[now]);
    now = Sson[now][f];
    // printf("!!!%llu %d %d\n", Sdate[now], now, Sson[now][0]);
    while(now && Sson[now][f^1]) {
        now = Sson[now][f^1];
    }
    return now;
}

void SDelete(char * x)//Удаление по значению
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

bool KVUpdate(char * k, char * v){//Обновление пары «ключ-значение»
    struct msg res = Tfind(k);
    if(!res.flag) return false;
    //ULL last = T.value[res.v];
    if(!Tupdate(k, v)) return false;
    // printf("tttttt--------\n");
    SDelete(last);
    // printf("????");
    SInsert(v);
    // printf("!!!!\n");
}



void work(int opt, FILE* f){
    FILE * fr = NULL;
    if(opt == 1) fr = fopen("tmp.save", "w");
    while(1){
        if(opt == 0) printf("input help to get info ...\n");
        char s[100];
        if(opt == 1) fscanf(f, "%s", s);
        else scanf("%s", s);
        // ULL k, v;     
        char *k, *v;           
        // char name[100];
        int k_len, kk_len, j;
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
                printf("please input quit to exit\n");
                //printf("length of 'filename' must < 100\n");
                printf("------------------------------------------------------\n");
                break;

            case 1:
                k = (char *)malloc(100);
                v = (char *)malloc(100);
                if(opt == 1) {
                    fscanf(f, "%s%s", k, v);
                    fprintf(fr, "save %s %s\n", k, v);
                }
                else {
                    scanf("%s%s", k, v);
                    fprintf(f, "save %s %s\n", k, v);
                }
                k_len = strlen(k);
                if(k_len % 8 != 0) {
                    kk_len = (k_len / 8) * 8 + 8;
                    k[kk_len] = 0;
                    for(j = kk_len - 1; j >= 0; j --) {
                        if(k_len >= 0)
                           k[j] = k[k_len --];
                        else k[j] = '0';
                    }
                }
                Tinsert(k, v, 0);
                //printf("!?!?!?%llu\n", Sson[0][0]);
                // printf("???%s\n", v);
                SInsert(v);
                //printf("!?!?!?%llu\n", Sson[0][0]);
                break;
            
            case 2:
                k = (char *)malloc(100);
                if(opt == 1) fscanf(f, "%s", k);
                else scanf("%s", k);
                k_len = strlen(k);
                if(k_len % 8 != 0) {
                    kk_len = (k_len / 8) * 8 + 8;
                    k[kk_len] = 0;
                    for(j = kk_len - 1; j >= 0; j --) {
                        if(k_len >= 0)
                           k[j] = k[k_len --];
                        else k[j] = '0';
                    }
                }
                struct msg res = Tfind(k);
                if(res.flag) {
                    printf(">the value is %s\n", res.v);
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
                v = (char *)malloc(100);
                if(opt == 1) fscanf(f, "%s", v);
                else scanf("%s", v);
                if(Ssize[Sroot] <= 0){
                    printf("data empty\n");
                } else {
                    int ans = SNext(v, 0);
                    // printf("---> ans %d\n", ans);
                    if(ans == 0) printf("no one is smaller than %s\n", v);
                    else printf(">the value is %s\n", Sdate[ans]);
                }
                break;
            case 6:
                v = (char *)malloc(100);
                if(opt == 1) fscanf(f, "%s", v);
                else scanf("%s", v);
                if(Ssize[Sroot] <= 0){
                    printf("data empty\n");
                } else {
                    int ans = SNext(v, 1);
                    if(ans == 0) printf("no one is bigger than %s\n", v);
                    else printf(">the value is %s\n", Sdate[ans]);
                }
                break;
            case 7:
                k = (char *)malloc(100);
                v = (char *)malloc(100);
                if(opt == 1) {
                    fscanf(f, "%s%s", k, v);
                    fprintf(fr, "update %s %s\n", k, v);
                }
                else {
                    scanf("%s%s", k, v);
                    fprintf(f, "update %s %s\n", k, v);
                }
                k_len = strlen(k);
                if(k_len % 8 != 0) {
                    kk_len = (k_len / 8) * 8 + 8;
                    k[kk_len] = 0;//结束符
                    for(j = kk_len - 1; j >= 0; j --) {
                        if(k_len >= 0)
                           k[j] = k[k_len --];
                        else k[j] = '0';
                    }
                }
                KVUpdate(k, v);
                break;
            case 8:
                k = (char *)malloc(100);
                if(opt == 1) fscanf(f, "%s", k);
                else scanf("%s", k);
                k_len = strlen(k);
                if(k_len % 8 != 0) {
                    kk_len = (k_len / 8) * 8 + 8;
                    k[kk_len] = 0;
                    for(j = kk_len - 1; j >= 0; j --) {
                        if(k_len >= 0)
                           k[j] = k[k_len --];
                        else k[j] = '0';
                    }
                }
                Tdelete(k);
                break;       
            case 9:
                if(opt == 1) fclose(fr);
                return ;
                break;
            default:
                printf(">unknow operation\n");
                break;
        }
    }
}

int main(){
    tree_init();
    printf("do you want to load from file?(yes or no)");
    char opt[10];
    scanf("%s", opt);
    char a[10] = "yes", b[10] = "no";
    // char aa[10] = "111", bb[10] = "00011";
    // printf("!!!%d\n", judge(aa, bb));
    if(check(opt, a) == true) {
        FILE* f = fopen("save.save", "r");
        if(f == NULL){
            printf("error while open file\n");
        } else {
            work(1, f);
            fclose(f);
            remove("save.save");
            system("cp tmp.save save.save");
            remove("tmp.save");
            f = fopen("save.save", "a");
            work(0, f);
            fprintf(f, "quit\n");
            fclose(f);
        }
    } else {
        FILE * f = fopen("save.save", "w");
        work(0, f);
        fprintf(f, "quit\n");
        fclose(f);
    }

    return 0;
}

