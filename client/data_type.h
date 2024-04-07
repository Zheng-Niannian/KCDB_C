#ifndef KVFILESERVER_DATA_TYPE_H
#define KVFILESERVER_DATA_TYPE_H

#define SOCKET_BUFFER_SIZE (1024)
#define SOCKET_DATA_HANDLER_SIZE (20)

typedef unsigned int uint32;

enum TransferPacketType{
    NOT_WAIT,LOGIN_REQUEST,LOGIN_RESULT,QUERY_KEY_REQUEST,QUERY_KEY_RESULT
};

typedef struct {
    uint32 dataType;
}PacketHeader;

typedef struct{
    void *src;
    uint32 actualLen;
    uint32 dataType;
}PacketPayload;

typedef struct{
    char username[20];
    char password[20];
}LoginRequest;

typedef struct{
    int loginResult;
    uint32 clientId;
}LoginResult;

typedef struct{
    uint32 keyLength;
    char *key;
}QueryKeyRequest;

typedef struct{
    uint32 valueLength;
    char *value;
}QueryKeyResult;



#endif //KVFILESERVER_DATA_TYPE_H

