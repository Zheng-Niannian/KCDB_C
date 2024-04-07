#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "transfer.h"

int server_socket;
struct sockaddr_in server_address, accept_address;
uint32 nextClientId=1;

void afterHandleMessage(ClientTransferState *state){
    state->messageType=NOT_WAIT;
    state->unknownMessageLength=0;
    state->unknownMessageType=0;
    state->messageLength=0;
    state->messageId=0;
    state->receivedLength=0;
    state->busy=0;
    if(state->buffer){
        free(state->buffer);
        state->buffer=NULL;
    }
}

extern void handleMessage(ClientTransferState *state,PacketPayload *payload);

const enum TransferPacketType immutableList[]={
        NOT_WAIT,LOGIN_REQUEST,LOGIN_RESULT
};
#define IMMUTABLE_LIST_SIZE     (3)
const int immutableTransferTypeLength[]={
        0,sizeof(LoginRequest),sizeof(LoginResult)
};

void handle_message(char* data, int socket, uint32 count,ClientTransferState* state) {
    char* handleDataAwait = NULL;//存储待处理的数据
    int handleDataAwaitCount = 0; //存储待处理的数据的长度

    printf("state.messageType:%d\n",state->messageType);
    if(state->messageType==NOT_WAIT){
        if(state->buffer==NULL&&count<sizeof(PacketHeader)){
            state->unknownMessageType=1;
            state->buffer=malloc(sizeof(PacketHeader));
            memset(state->buffer,0,sizeof(PacketHeader));
            memcpy(state->buffer,data,count);
            state->receivedLength=count;
            fprintf(stdout,"find small piece,size less than PacketHeader,copy %u bytes\n",count);
            return;
        }
        if(state->buffer!=NULL&&count<sizeof(PacketHeader)){
            if(state->receivedLength+count>=sizeof(PacketHeader)){

                memcpy(state->buffer+state->receivedLength,data,sizeof(PacketHeader)-state->receivedLength);
                PacketHeader *header=(PacketHeader*)state->buffer;
                state->messageType=header->dataType;
                fprintf(stdout,"find small piece,construct header success,message type:%u\n",state->messageType);
                state->unknownMessageType=0;
                char *previousBuffer=state->buffer;
                state->buffer=malloc(sizeof(PacketHeader)*2);
                memcpy(state->buffer,previousBuffer,sizeof(PacketHeader));
                free(previousBuffer);

                int useCount=sizeof(PacketHeader)-state->receivedLength;
                state->receivedLength=sizeof(PacketHeader);
                data+=useCount;
                count-=useCount;
                state->unknownMessageLength=1;
            }else{
                fprintf(stdout,"find small piece,construct header not finish\n");
                memcpy(state->buffer+state->receivedLength,data,count);
                state->receivedLength+=count;
                return;
            }
        }
    }


    if (state->messageType != NOT_WAIT) {
//        uint32 lastActionType = -1;
        int previousStateReceivedCount=state->receivedLength;
        state->receivedLength += count;
        printf("[INFO]Received stream data, count:%d, totalCount:%d, totalLength:%d\n", count, state->receivedLength,
               state->messageLength);

        if(state->unknownMessageLength){
            if(state->receivedLength>=sizeof(PacketHeader)*2){
                memcpy(state->buffer+previousStateReceivedCount,data,sizeof(PacketHeader)*2-previousStateReceivedCount);
                PacketHeader *payloadStart=(PacketHeader*)(state->buffer+sizeof(PacketHeader));
                state->messageLength=sizeof(PacketHeader)+payloadStart->dataType+sizeof(PacketHeader);
                fprintf(stdout,"recalculate message length finish,message length:%u\n",state->messageLength);
                state->unknownMessageLength=0;
                char*previousBuffer=state->buffer;
                state->buffer=malloc(state->messageLength);
                memset(state->buffer,0,state->messageLength);
                memcpy(state->buffer,previousBuffer,sizeof(PacketHeader)*2);
                free(previousBuffer);

                int useCount=sizeof(PacketHeader)*2-previousStateReceivedCount;
                count-=useCount;
                data+=useCount;
                printf("use count:%d\n",useCount);


            }else{
                memcpy(state->buffer+previousStateReceivedCount,data,count);
                fprintf(stdout,"cannot recalculate message length\n");
                return;
            }
        }

        if (state->receivedLength > state->messageLength) {
            int allowance = state->messageLength - (state->receivedLength - count);
            //TODO: Big file transfer,cannot use memory buffer directly,should use tmp file,need implement resource manager
            memcpy(state->buffer + state->receivedLength - count, data, allowance);
            uint32 remain_data = state->receivedLength - state->messageLength;
            state->receivedLength = state->messageLength;


            printf("current packet receive finish, remain data count:%d\n", remain_data);
            int nextPackageDataIndex = count - remain_data;
            handleDataAwait = (char *) malloc(sizeof(char) * remain_data);
            memcpy(handleDataAwait, data + nextPackageDataIndex, remain_data);
            handleDataAwaitCount = remain_data;
        } else {
            int writeIndex=state->receivedLength - count;
            printf("write index:%d\n",writeIndex);
            memcpy(state->buffer + state->receivedLength - count, data, count);
        }
        if (state->receivedLength == state->messageLength) {
            printf("[SUCCESS]Receive stream data completed\n");
//            lastActionType = state->messageType;
            printf("in handle message,before deserialize,message length=%d\n",state->messageLength);
            for(int i=0;i<state->messageLength;i++){
                printf("%u ",state->buffer[i]);
            }
            printf("\n");
            PacketPayload payload=deserializeActualData(state,state->buffer,state->messageLength);
            state->messageType = NOT_WAIT;
            printf("in handle message,after deserialize,message length=%d\n",state->messageLength);
            for(int i=0;i<state->messageLength;i++){
                printf("%u ",state->buffer[i]);
            }
            printf("\n");
            handleMessage(state,&payload);

            afterHandleMessage(state);
            free(state->buffer);
        }
    }
    else{
        PacketHeader* header=(PacketHeader*)data;

        printf("check immutable_list\n");
        int process=0;
        for(int i=0;i<IMMUTABLE_LIST_SIZE;i++){
            if(header->dataType==immutableList[i]){
                int needPayloadBytesCount=immutableTransferTypeLength[i]-(count-sizeof(PacketHeader));
                if(needPayloadBytesCount<=0){
                    PacketPayload payload=deserializeActualData(state,data,count);
                    process=1;
                    handleMessage(state,&payload);
                    afterHandleMessage(state);
                }else{
                    state->messageType=header->dataType;
                    state->buffer=malloc(immutableTransferTypeLength[i]+sizeof(PacketHeader));
                    state->messageLength=immutableTransferTypeLength[i]+sizeof(PacketHeader);
                    state->receivedLength=count;
                    memcpy(state->buffer,data,state->receivedLength);
                    //wait for reconstruct
                }
                int allowance=count-immutableTransferTypeLength[i]-sizeof(PacketHeader);
                if(allowance>0){
                    handleDataAwait=(char*)malloc(sizeof(char)*allowance);
                    memcpy(handleDataAwait,data+sizeof(PacketHeader)+immutableTransferTypeLength[i],allowance);
                    handleDataAwaitCount=allowance;
                }
            }
        }
        if(!process){
            printf("header.dataType:%d\n",header->dataType);
            if(header->dataType==QUERY_KEY_RESULT){
                if(count-sizeof(PacketHeader)>=4){
                    PacketHeader *payloadStart=(PacketHeader*)(data+sizeof(PacketHeader));
                    uint32 valueLength=payloadStart->dataType;
                    if((valueLength+sizeof(PacketHeader)*2)<count){
                        PacketPayload payload= deserializeActualData(state,data,count);
                        handleMessage(state,&payload);
                        afterHandleMessage(state);
                        int allowance=count-valueLength-sizeof(PacketHeader)*2;
                        if(allowance>0){
                            handleDataAwait=(char*)malloc(sizeof(char)*allowance);
                            memcpy(handleDataAwait,data+sizeof(PacketHeader)*2+valueLength,allowance);
                            handleDataAwaitCount=allowance;
                        }
                    }else{
                        state->messageType=header->dataType;
                        state->messageLength=sizeof(PacketHeader)*2+valueLength;
                        state->receivedLength=count;
                        state->buffer=malloc(state->messageLength);
                        memcpy(state->buffer,data,state->receivedLength);
                    }
                }else{
                    state->messageType=header->dataType;
                    state->unknownMessageLength=1;
                    state->buffer=malloc(sizeof(PacketHeader)*2);
                    memset(state->buffer,0,sizeof(PacketHeader)*2);
                    memcpy(state->buffer,data,count);
                }
            }
            if(header->dataType==QUERY_KEY_REQUEST){
                if(count-sizeof(PacketHeader)>=4){
                    PacketHeader *payloadStart=(PacketHeader*)(data+sizeof(PacketHeader));
                    uint32 keyLength=payloadStart->dataType;
                    if((keyLength + sizeof(PacketHeader) * 2) <= count){
                        PacketPayload payload= deserializeActualData(state,data,count);
                        handleMessage(state,&payload);
                        afterHandleMessage(state);
                        int allowance= count - keyLength - sizeof(PacketHeader) * 2;
                        if(allowance>0){
                            printf("allowance>0\n");
                            handleDataAwait=(char*)malloc(sizeof(char)*allowance);
                            memcpy(handleDataAwait, data +sizeof(PacketHeader)*2 + keyLength, allowance);
                            handleDataAwaitCount=allowance;
                        }
                    }else{
                        state->messageType=header->dataType;
                        state->messageLength=sizeof(PacketHeader)*2 + keyLength;
                        state->receivedLength=count;
                        state->buffer=malloc(state->messageLength);
                        memcpy(state->buffer,data,state->receivedLength);
                    }
                }else{
                    state->messageType=header->dataType;
                    state->unknownMessageLength=1;
                    state->buffer=malloc(sizeof(PacketHeader)*2);
                    memset(state->buffer,0,sizeof(PacketHeader)*2);
                    memcpy(state->buffer,data,count);
                }
            }
        }
    }

    if (handleDataAwait != NULL) {
        handle_message(handleDataAwait, socket, handleDataAwaitCount, state);
        free(handleDataAwait);
    }
}

void *clientThread(void *arg) {
    char buffer[1024]={0};
    ClientTransferState state=*(ClientTransferState*)arg;
    free(arg);
    while(state.connect) {
        int len = recv(state.socket, buffer, SOCKET_BUFFER_SIZE, 0);
        if (len > 0) {
            printf("receive message length: %d\n", len);
            handle_message(buffer,state.socket,len,&state);
//            memset(buffer, 0, sizeof(buffer)); // Clear buffer after printing
        }
    }
}

_Noreturn void initServer(void) {

    socklen_t len = sizeof(struct sockaddr_in);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    if (server_socket < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8005);
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    //TODO move into accept thread;
    while (1) {
        char buffer[SOCKET_BUFFER_SIZE] = { 0 };

        int newClient = accept(server_socket, (struct sockaddr*)&accept_address, &len);
        int receive = recv(newClient, buffer, SOCKET_BUFFER_SIZE, 0);
        if (receive <= 0) {
            perror("recv failed");
            continue;
        }

        ClientTransferState * state = (char*)malloc(sizeof(ClientTransferState));
        if (!state) {
            fprintf(stdout,"allocate memory failed,abort connection\n");
            close(newClient);
            continue;
        }
        memset(state,0,sizeof(ClientTransferState));
        state->socket=newClient;
        state->clientId=0;

        PacketPayload payload=deserializeActualData(state,buffer,receive);
        if(!payload.src){
            fprintf(stdout,"unable to deserialize login request\n");
            close(newClient);
            continue;
        }

        LoginRequest loginRequest=*(LoginRequest *)payload.src;

        LoginResult result={
                0,0
        };

        if(strcmp(loginRequest.username,loginRequest.password)==0){
            result.loginResult=1;
            result.clientId=nextClientId++;
        }

        state->clientId=result.clientId;

        fprintf(stdout,"login result:%u, clientId:%u\n",result.loginResult,result.clientId);

        char *retBuffer=serializeTransferData(&result,sizeof(LoginResult),LOGIN_RESULT);
        if(!retBuffer){
            fprintf(stdout,"serialize login result failed,abort connection\n");
            close(newClient);
            continue;
        }




        int sendData= sendSerializeData(state,retBuffer,sizeof(LoginResult));
        free(retBuffer);
        if(sendData<=0){
            fprintf(stdout,"unable to send login result,abort connection\n");
            close(newClient);
            continue;
        }

        if(!result.loginResult){
            fprintf(stdout,"login failed,incorrect username or password\n");
            close(newClient);
            continue;
        }

        state->connect=1;
        pthread_create(&state->threadId, NULL, clientThread, state);
        fprintf(stdout,"establish new connection\n");

    }
}

void *serializeTransferData(void* src, uint32 len, enum TransferPacketType type){

    for(int i=0;i<IMMUTABLE_LIST_SIZE;i++){
        if(type==immutableList[i]){
            char *buffer=(char*)malloc(sizeof(char)*len+sizeof(PacketHeader));
            if(!buffer){
                return NULL;
            }
            PacketHeader header={
                    .dataType=type
            };
            memcpy(buffer,&header,sizeof(PacketHeader));
            memcpy(buffer+sizeof(PacketHeader),src,len);
            return buffer;
        }
    }
    if(type==QUERY_KEY_REQUEST){
        QueryKeyRequest*request=(QueryKeyRequest*)src;
        int length=sizeof(PacketHeader)+sizeof(request->keyLength)+request->keyLength;
        char*buffer=(char*) malloc(sizeof(char)*length);
        if(!buffer){
            return NULL;
        }
        PacketHeader header={
                .dataType=type
        };
        memcpy(buffer,&header,sizeof(PacketHeader));
        memcpy(buffer+sizeof(PacketHeader),&request->keyLength,sizeof(request->keyLength));
        memcpy(buffer+sizeof(PacketHeader)+sizeof(request->keyLength),request->key,request->keyLength);
        return buffer;
    }
    if(type==QUERY_KEY_RESULT){
        QueryKeyResult *result=(QueryKeyRequest*)src;
        int length= sizeof(PacketHeader) + sizeof(result->valueLength) + result->valueLength;
        char*buffer=(char*) malloc(sizeof(char)*length);
        if(!buffer){
            return NULL;
        }
        PacketHeader header={
                .dataType=type
        };
        memcpy(buffer,&header,sizeof(PacketHeader));
        memcpy(buffer+sizeof(PacketHeader), &result->valueLength, sizeof(result->valueLength));
        memcpy(buffer+sizeof(PacketHeader)+sizeof(result->valueLength), result->value, result->valueLength);
        return buffer;
    }
    return NULL;
}

int sendSerializeData(ClientTransferState*state,void *serializeData,uint32 beforeSerializeLength){
    size_t bytesRead=0;
    size_t totalBytesSent = 0;
    size_t sendResult;

    size_t currentIndex=0;
    bytesRead=(beforeSerializeLength+sizeof(PacketHeader))>SOCKET_BUFFER_SIZE
              ?SOCKET_BUFFER_SIZE:beforeSerializeLength+sizeof(PacketHeader);


    while(bytesRead!=0){
        printf("send ...\n");
        sendResult=send(state->socket,serializeData+currentIndex,bytesRead,0);
        totalBytesSent+=sendResult;
        if(sendResult<bytesRead){
            fprintf(stdout,"Not all data sent. Sent %zu out of %zu bytes.",sendResult,bytesRead);
            state->connect=0;
            fprintf(stdout,"invalid data transfer,abort connection.");
            return -1;
        }
        currentIndex+=sendResult;
        bytesRead=(currentIndex+SOCKET_BUFFER_SIZE)<(beforeSerializeLength+sizeof(PacketHeader))
                  ?SOCKET_BUFFER_SIZE:(beforeSerializeLength+sizeof(PacketHeader)-currentIndex);
    }
    return totalBytesSent;
}

PacketPayload deserializeActualData(ClientTransferState*state,void* header, uint32 len){
    if(state->messageType==NOT_WAIT){//no need for packet reconstruct

    }
    char *str=header;
    for(int i=0;i<len;i++){
        printf("%d ",str[i]);
    }
    printf("\n");

    PacketHeader packetHeader=*(PacketHeader*)header;
    PacketPayload ret={
            NULL,0,-1
    };
    if(packetHeader.dataType<NOT_WAIT||packetHeader.dataType>QUERY_KEY_RESULT){
        printf("invalid data type,abort resolve");
        return ret;
    }
    for(int i=0;i<IMMUTABLE_LIST_SIZE;i++){
        if(packetHeader.dataType==immutableList[i]){
            ret.dataType=packetHeader.dataType;
            ret.actualLen=len-sizeof(PacketHeader);
            ret.src=(header+sizeof(PacketHeader));
//
//                if(len-sizeof(PacketHeader) >= immutableTransferTypeLength[i]){
//                    ret.dataType=packetHeader.dataType;
//                    ret.actualLen=len-sizeof(PacketHeader);
//                    ret.src=(header+sizeof(PacketHeader));
//                }else{
//                    state->messageType=packetHeader.dataType;
//                    //payload length
//                    state->messageLength=immutableTransferTypeLength[i];
//                    state->receivedLength=immutableTransferTypeLength[i]-(len-sizeof(PacketHeader));
//                    state->buffer=(char*)malloc(sizeof(state->messageLength));
//                    memcpy(state->buffer,header+sizeof(PacketHeader),state->receivedLength);
//                }
            return ret;
        }
    }
    if(packetHeader.dataType==QUERY_KEY_RESULT){
        fprintf(stdout,"find query key result\n");
        ret.dataType=packetHeader.dataType;
        QueryKeyResult *result=(QueryKeyResult*)(header+sizeof(PacketHeader));

        ret.actualLen=result->valueLength+sizeof(result->valueLength);
        ret.src=header + sizeof(PacketHeader);
        return ret;
    }
    if(packetHeader.dataType==QUERY_KEY_REQUEST){
        fprintf(stdout,"find query key request\n");
        ret.dataType=packetHeader.dataType;
        QueryKeyRequest *request=(QueryKeyRequest *)(header + sizeof(PacketHeader));

        ret.actualLen= request->keyLength + sizeof(request->keyLength);
        ret.src=header + sizeof(PacketHeader);
        return ret;
    }
    return ret;

}
