#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>



int save_use;

#define maxn 400000
const char op_[][12] = {{"help"}, {"save"}, {"find"}, {"savefile"}, {"load"}, {"find_less"}, {"find_more"}, {"update"}, {"delete"}, {"quit"}};


bool check(const char *a, const char *b){
    int len_a = strlen(a);
    int len_b = strlen(b);
    if(len_a != len_b) return false;
    // printf("check %s %s\n", a, b);
    int i;
    for(i = 0; i < len_a; i ++){
        // printf("check %d %c\n", i, a[i]);
        if(a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

int get_type(char *s, int opt){
    // printf("!!!%s\n", s);
    // int len = 10, i;
    int i; int len;
    if(opt == 0) len = 10;
    for(i = 0; i < len; i ++){
        if(check(op_[i], s)) return i;
    }
    // printf("???");
    return -1;
}


#define ULL unsigned long long 
void SDelete(char * x);
struct msg{
    char *v;
    bool flag;
};
struct Node * new_node(int type);
struct Node4{
    int tot;
    int key[4];//Значение ключа
    struct Node* son[4];//Получение значения узла-потомка в соответствующем месте.
};
void node4_init(struct Node4 *p){//Инициализация.
    p->tot = 0;
    int i;
    for(i = 0; i < 4; i ++) p->key[i] = -1;
}
void node4_insert(struct Node4 *p, int key, struct Node * pos){//Вставка в существующий узел или создание нового узла.
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
    char key[256];//К указывает на позицию дочернего узла
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
    short end;//Флаг, указывающий, является ли узел конечным (завершающим) узлом.
    struct Node * end_pos;//Указатель на узел с хранимым значением.

    struct Node * son;//Указатель на дочерний узел сжатого узла.
    char *s;//"Указатель на строку, хранящуюся в сжатом узле."
    int have;//Сжатие длины строки внутри узла.
};


void node_init(struct Node *p, int type){//Инициализация узла
    p->type = type;
    // printf("????%d\n", p->type);
    p->end = 0;
    p->end_pos = NULL;
    p->node4 = NULL, p->node16 = NULL, p->node48 = NULL, p->node256 = NULL;
    p->s = NULL;
    p->val = NULL;
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
    case 5://Это узел с хранимым значением.
        p->val = 0;
        break;
    default:
        printf(">err: type not exist!\n");
        return ;
        break;
    }
    p->s = 0;
    p->have = 0;//node4~node256могут быть сжатыми узлами.
}

void node_free(struct Node *p){//Освобождение памяти, занимаемой узлом.
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

int node_size(struct Node *p){//Получение количества дочерних узлов узла.
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

void tree_init(){//Инициализация дерева
    root = new_node(1);
    // printf("!!!!!!%d\n",root->type);
}

void expansion(struct Node * x){//Расширение узла.
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

void reduce(struct Node *x){//Уменьшение размера узла.
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

/*
Функция “match”, предназначенная для сопоставления строки внутри сжатого узла
1. Попытка последовательного сопоставления входной строки rest внутри сжатого узла.
2. В случае успешного сопоставления оценить, требуется ли разделение текущего сжатого узла в зависимости от результата сопоставления.
3. Если разделение необходимо, создается новый узел в качестве дочернего у текущего узла, и выполняются соответствующие операции по разделению узла.
*/

bool match(struct Node * now, char *rest, int *pos, int type){//"type = 1" означает, что если встречается несоответствие или дочерний узел пуст, функция немедленно возвращает false. Это используется в функции find.
    int i;
    char *ma = now->s; //Сжатая строка текущего узла.
	int tmp = now->have;//Длина строки текущего узла.
    for(i = *pos; i < strlen(rest); i += 8){//Последовательное сопоставление между сжатой строкой узла и входной строкой.
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

    if(tmp == 0){//Полное внутреннее совпадение узла.
        if(i >= strlen(rest)){//Также полное совпадение с rest.
            if(type == 1){//rest полностью совпал, но текущий узел не является узлом с хранимым значением, поэтому узел не найден
                return false;
            }
            struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));//Создание нового пустого узла.
            newNode->type = now->type;
            newNode->node4 = now->node4;
            newNode->node16 = now->node16;
            newNode->node48 = now->node48;
            newNode->node256 = now->node256;//Новый узел наследует все дочерние узлы от текущего узла.
            newNode->have = 0;
            now->have --;
            int s_len = strlen(now->s);
            now->s[s_len - 8] = 0;//“\0”
            now->type = 1;
            now->node4 = (struct Node4 *)malloc(sizeof(struct Node4));//Очистка исходных дочерних узлов текущего узла и присоединение новых дочерних узлов.
            node4_init(now->node4);
            now->node16 = NULL;
            now->node48 = NULL;
            now->node256 = NULL;
            *rest %= (1ll << 8);//Только последний фрагмент остается в rest.
            *pos = strlen(rest) - 8;
            node4_insert(now->node4, *rest, newNode);//Присоединение нового узла в качестве дочернего узла к текущему узлу.
            return false;//Позволяет функции insert продолжить движение вниз от текущего узла.
        } else {
            *pos = i;
            if(type == 1) return true;//Позволяет функции find продолжить движение вниз.
            return false;//Позволяет функции insert продолжить движение вниз от текущего узла.
        }
    } else {//Частичное внутреннее несовпадение узла
        if(i >= strlen(rest) && type == 1){//rest полностью совпал, но текущий узел не является узлом с хранимым значением, поэтому ключ не найден.
            return false;
        }
        struct Node * newNode = (struct Node *)malloc(sizeof(struct Node));//Создание нового сжатого узла.
        newNode->type = now->type;
        newNode->node4 = now->node4;
        newNode->node16 = now->node16;
        newNode->node48 = now->node48;
        newNode->node256 = now->node256;//Новый узел наследует все дочерние узлы от текущего узла.
        newNode->have = tmp - 1;
        newNode->val = NULL;
        newNode->s = NULL;
        if(newNode->have > 0) {
            newNode->s = (char *)malloc((newNode->have) * 8 + 1);//Необходимо добавить дополнительный символ для размещения символа окончания
            newNode->s[newNode->have * 8] = 0;
            int rev = now->have - newNode->have;
            rev *= 8;
            int j;
            for(j = 0; j < newNode->have * 8; j ++){
                newNode->s[j] = now->s[rev + j];//Переместить удаленный фрагмент в новый узел.
            }
        }
        int key_t = (now->have - newNode->have - 1) * 8;
        int key = 0;
        int j;
        for(j = 0; j < 8; j ++) 
			key = (key << 1) + now->s[key_t + j] - '0';//Извлечение фрагмента, который будет использоваться в качестве ключа.
        now->type = 1;
        now->node4 = (struct Node4 *)malloc(sizeof(struct Node4));
        node4_init(now->node4);
        now->node16 = NULL;
        now->node48 = NULL;
        now->node256 = NULL; 
        if(i >= strlen(rest)){//rest полностью сопоставлен, требуется разделение на промежуточный узел в качестве узла с хранимым значением.
            struct Node * newMidNode = new_node(1);
            int midkey = 0;
            for(j = strlen(rest) - 8; j < strlen(rest); j ++) midkey = (midkey << 1) + rest[j] - '0';

            now->s[(now->have - newNode->have - 1 - 1) * 8] = 0;//Удаление фрагмента, который был использован в качестве ключа, а также удаление фрагмента, который был выделен и используется в узле с хранимым значением.
            now->have -= newNode->have + 1 + 1;
            *pos = strlen(rest) - 8;
            node4_insert(now->node4, midkey, newMidNode);     
            node4_insert(newMidNode->node4, key, newNode);
            return false;
        } 
        now->s[(now->have - newNode->have - 1) * 8] = 0;
        now->have -= newNode->have + 1;
        //"rest не полностью сопоставлен."
        node4_insert(now->node4, key, newNode);//Добавление нового узла в качестве нового дочернего узла текущего узла.
        *pos = i;
        return false;
    }
}

bool Tinsert(char * k, char * v, int flag){//Вставка.
    // printf("--------------------insert\n");
    struct Node * now = root; 
    char rest[100];
    memcpy(rest, k, sizeof(rest));
    int rest_len = strlen(rest);
    int i;
    int stay = 0;
    for(i = 0; i < rest_len; i += 8){//В качестве единицы используется блок из 8 бит.
        // printf("--->%d %d %llu now: %p\n", i, x, rest, now);
        int j;int done = 0;
        // printf("---- %d\n", now->type);
        if(now->have >= 1 && stay == 0){//Если текущий узел является сжатым узлом и это первое посещение данного узла.
            match(now, rest, &i, 0);
        }
        stay = 0;
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
                if(now->node4->tot == 4){//Узел заполнен.
                    expansion(now);//Расширение узла.
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
                } else {//Вставка нового узла.
                    if(now != root && now->node4->tot == 0 && now->end == 0 && i + 8 < rest_len){
						//Если текущий узел не имеет дочерних узлов, и не является корневым узлом,
						//и не является узлом с хранимым значением,и новый узел также не будет узлом с хранимым значением,
						//то просто объединить новый узел с текущим узлом.
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
                        tmp[tt + 8] = 0;//Присвоение значения '\0' в конце.
                        now->have ++;
                        stay = 1;//Поскольку следующий узел был сжат в текущий узел, это эквивалентно возвращению к предыдущему узлу следующего узла. Внутри узла уже произведено сопоставление и не может быть повторено.
                        break;
                    } else {//Иначе создается новый узел.
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
    if(now->end == 1) {//"Узел с хранимым значением."
        if(flag == 0) {
            printf(">k = %s is exist, do you mean to update the value %s?", k, now->end_pos->val);// the value has been overwritten!\n", k);
            return false;
        }
        else {
            now = now->end_pos;
            now->val = v;
            // memcpy(now->val, v, sizeof(now->val));
            printf(">k = %s is exist, the value has been overwritten!\n", k);
            return true;
        }
    }
    else {//create new val node
        if(flag == 0){
            now->end = 1;
            now->end_pos = new_node(5);
            now = now->end_pos;
            now->val = v;
            // memcpy(now->val, v, sizeof(now->val));
            return true;
        } else {
            printf(">k - %s is absent?\n", k);
            return false;
        }
        
    }
}

struct msg Tfind(char * k){
    struct Node * now = root; char rest[100];int last = 0;
    int i;
    memcpy(rest, k, sizeof(rest));
    // printf("-----------------find\n");//
    for(i = 0; i < strlen(rest); i += 8){
        // printf("%d \n", i);
        int err = 0;
        int j;int done = 0;
        if(now->have >= 1){
            if(match(now, rest + i, &i, 1) == false) {
                printf(">k = %s is absent!\n", k);
                return (struct msg){NULL, false};
            }
        }
        // int x = rest / (1ll << i);
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

    }
    if(now->end == 0) {
        printf(">k = %s is absent\n", k);
        return (struct msg){NULL, false};
    }
    now = now->end_pos;
    return (struct msg){now->val, true};       
}

bool Tupdate(char * k, char * v){//Изменение значения v.
    struct msg res = Tfind(k);
    if(res.flag == false){// k не существует.
        return false;
    } else Tinsert(k, v, 1);
    return true;
}

struct Node * last[10000];
int way[10000];
int top;
bool Tdelete(char *k){
    struct msg res = Tfind(k);
    if(res.flag == false){
        printf(">k = %s is absent!\n", k);
        return false;
    } 

    struct Node * now = root; 
    char rest[100];//Размер по умолчанию: 100.
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
        // rest %= (1ll << i);
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
        if(last[top]->end == 1) break;//Если текущий узел является узлом с хранимым значением.
        if(node_size(last[top]) >= 1) break;//Если текущий узел имеет дочерние узлы.
        struct Node * now = last[top - 1];//Удаление соответствующего дочернего узла в родительском узле текущего узла.
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
                    // printf("!!!!%d\n", now->type);
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
    return true;
}

//Это структура данных, отвечающая за поиск.
int Sroot, Sn, Stot;
int Ssize[maxn], Sf[maxn], Scnt[maxn], Sson[maxn][2];
char* Sdate[maxn];
// size - сумма cnt поддерева, в котором находится узел.
void Supdate(int x)
{
    Ssize[x]=Ssize[Sson[x][0]]+Ssize[Sson[x][1]]+Scnt[x];
}
void Srotate(int x)
{
    if(x == Sroot) return ;
    int y = Sf[x], z = Sf[y], k = Sson[y][1]==x;
    // printf("rotate %d %d\n", y, z);
    Sson[z][Sson[z][1] == y] = x, Sf[x] = z;
    Sson[y][k] = Sson[x][k^1];
    Sf[Sson[x][k^1]] = y;
    Sson[x][k^1] = y;
    Sf[y] = x;
    Supdate(y), Supdate(x);
}
void Ssplay(int x, int goal)//Растяжение
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
int judge(char * x, char * y){//"Сравнение двух бинарных строк: 1 - если x > y, 0 - если x = y, -1 - если x < y."
    int xx = strlen(x), yy = strlen(y);
    if(xx > yy) return 1;
    if(xx < yy) return -1;
    int i;
    for(i = 0; i < xx; i ++){
        if(x[i] < y[i]) return -1;
        if(x[i] > y[i]) return 1;
    }
    return 0;
}

void SInsert(char * x)//Вставка узла.
{
    int now = Sroot,fa=0;
    while(now && judge(Sdate[now], x) != 0)
    {
        fa = now;
        now = Sson[now][judge(x, Sdate[now]) >= 1];
    }
    if(now)Scnt[now]++;
    else 
    {
        now = ++Stot;
        if(fa) Sson[fa][judge(x, Sdate[fa]) >= 1] = now;
        Sson[Stot][0] = Sson[Stot][1] = 0;
        Sf[Stot] = fa, Sdate[Stot] = x;
        Scnt[Stot] = Ssize[Stot] = 1;
        
    }
    Ssplay(now,0);//Перемещение вверх.
    
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

void SDelete(char * x)//Удаление по значению.
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

bool KVUpdate(char * k, char * v){
    struct msg res = Tfind(k);//Сначала найдите оригинальное значение в хранилище.
    if(!res.flag) return false;
    if(!Tupdate(k, v)) return false;//Сначала измените значение v в хранилище.
    // printf("tttttt--------\n");
    SDelete(res.v);//Удаление оригинального значения из структуры поиска.
    // // printf("????");
    SInsert(v);//Вставка нового значения в структуру поиска.
    // printf("!!!!\n");
    return true;
}

char * tt;
void work(int opt, FILE* f){
    FILE * fr = NULL;
    char ans[10000];
    if(opt == 1) fr = fopen("tmp.save", "w");//Перезапись
    else fr = fopen("tmp.in", "r");
    // printf("!!!!!");
    // int count = 10;
    while(1){
        // if(opt == 0) printf("input help to get info ...\n");
        char s[100];
        // printf("!!??%d\n", f);
        if(opt == 1) fscanf(f, "%s", s);
        else {
            fscanf(fr, "%s", s);
        }
        printf("?!?%s\n", s);
        // ULL k, v;     
        char *k, *v;           
        // char name[100];
        int k_len, kk_len, j;
        switch (get_type(s, 0))
        {
            case 0:
                if(opt == 1) continue;
                // printf("------------------------------------------------------\n");
                // printf("input 'save k v' to save\n");
                // printf("input 'find k' to find v by k\n");
                // printf("input 'update k v' to update the value on a pair of (k, v)");
                // printf("input 'delete k' to delete by k\n");
                // // printf("input 'savefile filename' to save data into file\n");
                // // printf("input 'load filename' to load data from file\n");
                
                // printf("input 'find_less k' to find the closest smaller value by v\n");
                // printf("input 'find_more k' to find the closest bigger value by v\n");
                // printf("please input 'quit' to exit\n");
                // // printf("length of 'filename' must < 100\n");
                // printf("------------------------------------------------------\n");
                // char tmp[500] = "------------------------------------------------------\ninput 'save k v' to save\ninput 'find k' to find v by k\ninput 'update k v' to update the value on a pair of (k, v)\ninput 'delete k' to delete by k\ninput 'find_less k' to find the closest smaller value by v\ninput 'find_more k' to find the closest bigger value by v\nplease input 'quit' to exit\n------------------------------------------------------\n";
                strcpy(ans, "------------------------------------------------------\ninput 'save k v' to save\ninput 'find k' to find v by k\ninput 'update k v' to update the value on a pair of (k, v)\ninput 'delete k' to delete by k\ninput 'find_less k' to find the closest smaller value by v\ninput 'find_more k' to find the closest bigger value by v\nplease input 'quit' to exit\n------------------------------------------------------\n");
                break;

            case 1:
                k = (char *)malloc(100);
                v = (char *)malloc(100);
                if(opt == 1) {
                    fscanf(f, "%s%s", k, v);
                    fprintf(fr, "save %s %s\n", k, v);
                }
                else {
                    fscanf(fr, "%s%s", k, v);
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
                if(!Tinsert(k, v, 0)) {
                    strcpy(ans, "error");
                    break;
                };
                //printf("!?!?!?%llu\n", Sson[0][0]);
                // printf("???%s\n", v);
                SInsert(v);
                //printf("!?!?!?%llu\n", Sson[0][0]);
                strcpy(ans, "done");
                break;
            
            case 2:
                k = (char *)malloc(100);
                if(opt == 1) fscanf(f, "%s", k);
                else fscanf(fr, "%s", k);
                k_len = strlen(k);
                if(k_len % 8 != 0) {//Поскольку k разбивается на блоки по 8 бит, если блок составляет менее 8 бит, то в начале добавляются нули для дополнения.
                    kk_len = (k_len / 8) * 8 + 8;
                    k[kk_len] = 0;//\0
                    for(j = kk_len - 1; j >= 0; j --) {
                        if(k_len >= 0)
                           k[j] = k[k_len --];
                        else k[j] = '0';
                    }
                }
                struct msg res = Tfind(k);
                if(res.flag) {
                    // printf(">the value is %s\n", res.v);
                    strcpy(ans, ">the value is ");
                    strcat(ans, res.v);
                } else strcpy(ans, "not found\n");
                break;
            // case 3:
            //     scanf("%s", name);
            //     Tsave(name);
            //     break;
            // case 4:
            //     scanf("%s", name);
            //     Tload(name);
            //     break;          
            case 5:
                v = (char *)malloc(100);
                if(opt == 1) fscanf(f, "%s", v);
                else fscanf(fr, "%s", v);
                if(Ssize[Sroot] <= 0){
                    // printf("data empty\n");
                    strcpy(ans, "data empty\n");
                } else {
                    int ans_ = SNext(v, 0);
                    // printf("---> ans %d\n", ans);
                    if(ans == 0) {
                        strcpy(ans, "no one is smaller than ");
                        strcat(ans, v);
                    }
                    else {
                        strcpy(ans, ">the value is ");
                        strcat(ans, Sdate[ans_]);
                    }
                }
                break;
            case 6:
                v = (char *)malloc(100);
                if(opt == 1) fscanf(f, "%s", v);
                else fscanf(fr, "%s", v);
                if(Ssize[Sroot] <= 0){
                    strcpy(ans, "data empty\n");
                } else {
                    int ans_ = SNext(v, 1);
                    if(ans_ == 0) {
                        strcpy(ans, "no one is bigger than ");
                        strcat(ans, v);
                        //printf("no one is bigger than %s\n", v);
                    }
                    else {
                        strcpy(ans, ">the value is ");
                        strcat(ans, Sdate[ans_]);
                        //printf(">the value is %s\n", Sdate[ans]);
                    }
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
                    fscanf(fr, "%s%s", k, v);
                    fprintf(f, "update %s %s\n", k, v);
                }
                k_len = strlen(k);
                if(k_len % 8 != 0) {//Поскольку k разбивается на блоки по 8 бит, если блок составляет менее 8 бит, то в начале добавляются нули для дополнения.
                    kk_len = (k_len / 8) * 8 + 8;
                    k[kk_len] = 0;
                    for(j = kk_len - 1; j >= 0; j --) {
                        if(k_len >= 0)
                           k[j] = k[k_len --];
                        else k[j] = '0';
                    }
                }
                if(KVUpdate(k, v) == false){
                    strcpy(ans, "error");
                    break;
                }
                strcpy(ans, "done");
                break;
            case 8:
                k = (char *)malloc(100);
                if(opt == 1) {
                    fscanf(f, "%s", k);
                    fprintf(fr, "delete %s\n", k);
                }
                else {
                    fscanf(fr, "%s", k);
                    fprintf(f, "delete %s\n", k);//delete also need save
                }
                k_len = strlen(k);
                if(k_len % 8 != 0) {//Поскольку k разбивается на блоки по 8 бит, если блок составляет менее 8 бит, то в начале добавляются нули для дополнения.
                    kk_len = (k_len / 8) * 8 + 8;
                    k[kk_len] = 0;//"\0"
                    for(j = kk_len - 1; j >= 0; j --) {
                        if(k_len >= 0)
                           k[j] = k[k_len --];
                        else k[j] = '0';
                    }
                }
                if(Tdelete(k) == false){
                    strcpy(ans, "error");
                    break;
                }
                strcpy(ans, "done");
                break;
            case 9:
                if(opt == 1) fclose(fr);
                strcpy(ans, "quit");
                return ;
                break;
            default:
                strcpy(ans, ">unknow operation\n");//printf(">unknow operation\n");
                break;
        }
        if(opt == 0) {
            tt = ans;
            fclose(fr);
            break;
        }
    }
    printf("out of while\n");
}

#define port_ 1313
const unsigned int reuseable = 1;//Включение повторного использования адреса.

const char help_[] = {"help"};
const char save_[] = {"save"};
const char find_[] = {"find"};
const char savefile_[] = {"savefile"};
const char load_[] = {"load"};
void server_init(struct sockaddr_in *p){
    p -> sin_family = AF_INET;//ipv4
    p -> sin_port = htons(port_);
    p -> sin_addr.s_addr = htonl(INADDR_ANY);
}

void save_value(char *s){
    FILE* f = fopen("1.save", "a");
    fprintf(f, "%s\n", s);
    fclose(f);
}

int rest = 1;
int fd_server = -1;
int ad_len;

int get(char *buf, int fd_client){
    int ans = read(fd_client, buf, 1024);
    // printf("!????? %d %d %d\n", fd_client, buf, ans);
    return ans;
}

// int save_uses;
sem_t sem;

void* talk(void* fd_client_) {//Взаимодействие с клиентом.
    //int fd_client = (int) fd_client_;
    int fd_client = *((int*)fd_client_);
    printf("fd_client %d\n", fd_client);
    char buf[1024];//"Чтение в буфер."
    memset(buf, 0, sizeof(buf));
    // fd_client = accept(fd_server, (struct sockaddr *)&client_ad, &ad_len);
    if (fd_client < 0) {
        printf("tcp: fail to accept because of invalid client fd %d\n", fd_client);
        return NULL;
    }
    while (1) {
        if (rest >= 0) {
            rest--;
            break;
        }
    }
    printf("--- client %d connect\n", fd_client);

    while (1) {
        // int read_l = read(fd_client, buf, 1024);
        // int read_l = recv(fd_client, &buf, sizeof(buf), 0);
        int read_l = get(buf, fd_client);
        if (read_l < 0) {
            printf("error while reading\n");
            break;
        }
        if (read_l == 0) {//Чтение завершено.
            printf("--- Complete reception, client %d disconnect\n", fd_client);
            break;
        }
        printf("receive data from client %d: %s\n", fd_client, buf);
        sem_wait(&sem);
        FILE* f = fopen("save.save", "a");
        FILE* fr = fopen("tmp.in", "w");
        fprintf(fr, "%s", buf);
        fclose(fr);
        work(0, f);
        fclose(f);
        char err[] = "err";
        if (tt == NULL) tt = err;
        // memcpy(buf, tt, sizeof(buf));
        write(fd_client, tt, read_l);//Отправка клиенту информации о результате операции.
        printf("send data to client %d: %s\n", fd_client, tt);
        sem_post(&sem);
    }
    close(fd_client);
    rest++;
    pthread_exit(NULL);
    return NULL;
}


int tcp_(){//int tcp_(){//Использование протокола tcp  

    // int fd_client = -1;//идентификатор клиента
    printf("start tcp\n");
    fd_server = socket(AF_INET, SOCK_STREAM, 0);//Используйте протокол ipv4, TCP.
    if(fd_server < 0){
        printf("tcp: socket create failed\n");
        return -1;//Создание экземпляра не удалось
    }
    
    if(setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, (void *)& reuseable, sizeof(reuseable)) < 0){
        printf("tcp: fail to reuse\n");
        return -1;
    }

    struct sockaddr_in server_ad, client_ad;
    memset(&server_ad, 0, sizeof(server_ad));
    memset(&client_ad, 0, sizeof(client_ad));
    server_init(&server_ad);//"Инициализация настроек."

    ad_len = sizeof(server_ad);
    if(bind(fd_server, (const struct sockaddr *)&server_ad, ad_len) < 0){//Привязка адреса.
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
    int fd_client;//Идентификатор клиента.
    while(1){//Многопоточная обработка
        if(thread_tot >= 99) thread_tot = -1;//Ограничение максимального количества подключений клиентов.
        // int fd_client = -1;//Идентификатор клиента.
        fd_client = accept(fd_server, (struct sockaddr *)&client_ad, &ad_len);       
        //int ret_thread = pthread_create(&thread_id[++thread_tot], NULL, talk, (void *)fd_client);
		int ret_thread = pthread_create(&thread_id[++thread_tot], NULL, talk, (void*)&fd_client);
		if(-1 == ret_thread) printf("thread failed"); 
    }
    close(fd_server);
    
    return 0;
}

int main(){
    tree_init();
    sem_init(&sem, 0, 1);
    // printf("do you want to load from file?(yes or no)");
    // char opt[10];
    // scanf("%s", opt);
    // char a[10] = "yes", b[10] = "no";
    // char aa[10] = "111", bb[10] = "00011";
    // printf("!!!%d\n", judge(aa, bb));
    FILE* f = fopen("save.save", "a");//Только для чтения.
    fprintf(f, "\nquit");
    fclose(f);
    f = fopen("save.save", "r");//Только для чтения.
    if(f == NULL){
        printf("error while open file\n");
    } else {
        //printf("OOOO?%d\n", f);
		printf("OOOO?%p\n", (void*)f);

        work(1, f);//"Загрузка файла."
        fclose(f);
        remove("save.save");//Удалить исходный файл
        system("cp tmp.save save.save");
        remove("tmp.save");//Удаление временного файла.
    }

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
    return 0;
}
