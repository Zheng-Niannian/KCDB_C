#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "transfer.h"

ClientTransferState clientTransferState;

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
    //printf("data[0]:%d\n",data[0]);
    char* handleDataAwait = NULL;
    int handleDataAwaitCount = 0;

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
        }
    }
    
    if (handleDataAwait != NULL) {
        handle_message(handleDataAwait, socket, handleDataAwaitCount, state);
        free(handleDataAwait);
    }
}

void *sendMsg(void *arg) {
    char message[SOCKET_BUFFER_SIZE];
    while(clientTransferState.connect) {
        fprintf(stdout,"input querykey:");
        fgets(message, 1024, stdin);
        if(message[0]=='\n'){
            continue;
        }
        int length=strlen(message);
        QueryKeyRequest request={
                length,message
        };
        void* serializeData=serializeTransferData(&request,length,QUERY_KEY_REQUEST);
        if(sendSerializeData(&clientTransferState,serializeData,sizeof(request.keyLength)+length)<0){
            fprintf(stdout,"transfer error occurred\n");
        }
        free(serializeData);
    }
    fprintf(stdout,"sendMsg thread exit normally\n");
    return NULL;
}

void *receiveMsg(void *arg) {
    char buffer[SOCKET_BUFFER_SIZE]={0};
    while(clientTransferState.connect) {
        int len = recv(clientTransferState.socket, buffer, SOCKET_BUFFER_SIZE, 0);
        if (len > 0) {
            handle_message(buffer,clientTransferState.socket,len,&clientTransferState);
            memset(buffer, 0, sizeof(buffer)); // Clear buffer after printing
        }
    }
    fprintf(stdout,"receiveMsg thread exit normally\n");
    return NULL;
}

void initClient(void){
    memset(&clientTransferState,0,sizeof(ClientTransferState));
    clientTransferState.socket=socket(AF_INET, SOCK_STREAM, 0);
    if (clientTransferState.socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8005);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientTransferState.socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connect failed");
        exit(EXIT_FAILURE);
    }

    LoginRequest loginRequest;
    printf("input username\n");
    scanf("%s", loginRequest.username);
    printf("input password\n");
    scanf("%s", loginRequest.password);

    void* loginTransferData= serializeTransferData(&loginRequest, sizeof(LoginRequest), LOGIN_REQUEST);
    if(!loginTransferData){
        fprintf(stdout,"unable to serialize login request\n");
        exit(0);
    }
    int loginRequestLen= sendSerializeData(&clientTransferState,loginTransferData,sizeof(LoginRequest));
    free(loginTransferData);
    if(loginRequestLen<=0){
        fprintf(stdout,"send login request failed\n");
        exit(0);
    }

    char buffer[SOCKET_BUFFER_SIZE]={0};
    int loginResultLen= recv(clientTransferState.socket, buffer, SOCKET_BUFFER_SIZE, 0);
    if(loginResultLen<=0){
        fprintf(stdout,"recv login response failed\n");
        exit(0);
    }

    PacketPayload payload=deserializeActualData(&clientTransferState,buffer,loginResultLen);
    if(!payload.src){
        fprintf(stdout,"unable to deserialize login response\n");
        exit(0);
    }

    LoginResult loginResult=*(LoginResult*)payload.src;
    if(!loginResult.loginResult){
        fprintf(stdout,"login request refused\n");
        exit(0);
    }
    fprintf(stdout,"login success,get clientId:%u\n",loginResult.clientId);
    clientTransferState.clientId=loginResult.clientId;
    clientTransferState.connect=1;

    pthread_t receiveThreadId;
    pthread_create(&clientTransferState.threadId, NULL, sendMsg, NULL);
    pthread_create(&receiveThreadId, NULL, receiveMsg, NULL);

    pthread_join(clientTransferState.threadId, NULL);
    pthread_join(receiveThreadId, NULL);

    close(clientTransferState.socket);
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
    printf("send bytes count:%zu\n",totalBytesSent);

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
