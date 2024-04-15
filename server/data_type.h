#ifndef KVFILESERVER_DATA_TYPE_H
#define KVFILESERVER_DATA_TYPE_H

#include <bits/stdint-intn.h>
#include<stdio.h>

#define SOCKET_BUFFER_SIZE (1024)
#define SOCKET_DATA_HANDLER_SIZE (20)

enum CommandType{
    NOT_WAIT,LOGIN_REQUEST,LOGIN_RESULT,FIND_REQUEST,FIND_RESULT,SET_REQUEST,SET_RESULT,UPDATE_REQUEST,UPDATE_RESULT,
    FIND_LESS_REQUEST,FIND_LESS_RESULT,FIND_MORE_REQUEST,FIND_MORE_RESULT,DELETE_REQUEST,DELETE_RESULT,HELP_COMMAND
};



typedef struct {
    int32_t dataType;
}PacketHeader;

typedef struct{
    void *src;
    int32_t actualLen;
    int32_t dataType;
    FILE *fp;
}PacketPayload;

typedef struct{
    char username[20];
    char password[20];
}LoginRequest;

typedef struct{
    int loginResult;
    int32_t clientId;
}LoginResult;

//typedef struct{
//    int32_t keyLength;
//    void *key;
//}QueryKeyRequest;
//
//typedef struct{
//    int32_t valueLength;
//    void *value;
//}QueryKeyResult;

typedef struct{
    int totalLength;//sizeof(KeyLength)+sizeof(valueLength)+KeyContent+ValueContent
    int keyLength;
    int valueLength;
    //key content
    //value content;
}TransferCommand;



#endif //KVFILESERVER_DATA_TYPE_H

