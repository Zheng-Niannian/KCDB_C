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
    if(state->fp){
        fclose(state->fp);
        state->fp=NULL;
    }
}

extern void handleMessage(ClientTransferState *state,PacketPayload *payload);
extern void *user_interaction_thread(void *arg);

const enum CommandType immutableList[]={
        NOT_WAIT,LOGIN_REQUEST,LOGIN_RESULT
};
const int immutableTransferTypeLength[]={
        0,sizeof(LoginRequest),sizeof(LoginResult)
};
#define IMMUTABLE_LIST_SIZE     (3)



const enum CommandType serverHandleMessageTypeList[]={
        LOGIN_REQUEST,FIND_REQUEST,SET_REQUEST,UPDATE_REQUEST,FIND_LESS_REQUEST,FIND_MORE_REQUEST,DELETE_REQUEST
};
#define SERVER_HANDLE_MESSAGE_TYPE_LIST_SIZE    (7)

const enum CommandType serverReturnMessageTypeList[]={
        LOGIN_RESULT,FIND_RESULT,SET_RESULT,UPDATE_RESULT,FIND_LESS_RESULT,FIND_MORE_RESULT,DELETE_RESULT
};
#define SERVER_RETURN_MESSAGE_TYPE_LIST_SIZE     (7)


void handle_message(char* data, int socket, int32_t count,ClientTransferState* state) {

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

                for(int i=0;i<IMMUTABLE_LIST_SIZE;i++){
                    if(state->messageType==immutableList[i]){
                        state->unknownMessageLength=0;
                        state->messageLength=immutableList[i];
                    }
                }
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
                state->messageLength=sizeof(PacketHeader)*2+payloadStart->dataType;
                fprintf(stdout,"recalculate message length finish,message length:%u\n",state->messageLength);
                state->unknownMessageLength=0;
                char*previousBuffer=state->buffer;

                if(state->messageLength>TRANSFER_MEMORY_BUFFER_MAX_SIZE){
                    sprintf(state->filename_buffer,"/home/zheng1/KVF/tmpfile_buffer_%d_%d.bin",state->clientId,state->messageId++);
                    fprintf(stdout,"try to create file: %s\n",state->filename_buffer);
                    state->fp=fopen(state->filename_buffer,"w+");
                    if(!state->fp){
                        fprintf(stdout,"cannot create file buffer for client%d,abort connection\n",state->clientId);
                        state->connect=0;
                        return;
                    }
                    fwrite(state->buffer,1,sizeof(PacketHeader)*2,state->fp);
                    state->buffer=NULL;
                }else{
                    state->buffer=malloc(state->messageLength);
                    memset(state->buffer,0,state->messageLength);
                    memcpy(state->buffer,previousBuffer,sizeof(PacketHeader)*2);
                }

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
            if(state->buffer){
                memcpy(state->buffer + state->receivedLength - count, data, allowance);
            }else{
                fwrite(data,1,allowance,state->fp);
            }

            int32_t remain_data = state->receivedLength - state->messageLength;
            state->receivedLength = state->messageLength;
            printf("current packet receive finish, remain data count:%d\n", remain_data);
            int nextPackageDataIndex = count - remain_data;
            handleDataAwait = (char *) malloc(sizeof(char) * remain_data);
            memcpy(handleDataAwait, data + nextPackageDataIndex, remain_data);
            handleDataAwaitCount = remain_data;
        } else {
            int writeIndex=state->receivedLength - count;
            printf("write index:%d\n",writeIndex);

            if(state->buffer){
                memcpy(state->buffer + state->receivedLength - count, data, count);
            }else{
                fwrite(data,1,count,state->fp);
            }
        }
        if (state->receivedLength == state->messageLength) {
            printf("[SUCCESS]Receive stream data completed\n");
            printf("in handle message,before deserialize,message length=%d\n",state->messageLength);

            PacketPayload payload=deserializeActualData(state,state->buffer,state->messageLength);
            state->messageType = NOT_WAIT;
            printf("in handle message,after deserialize,message length=%d\n",state->messageLength);

            handleMessage(state,&payload);
            afterHandleMessage(state);
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

        }
    }
    
    if (handleDataAwait != NULL) {
        handle_message(handleDataAwait, socket, handleDataAwaitCount, state);
        free(handleDataAwait);
    }
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
    pthread_create(&clientTransferState.threadId, NULL, user_interaction_thread, NULL);
    pthread_create(&receiveThreadId, NULL, receiveMsg, NULL);
    pthread_join(clientTransferState.threadId, NULL);
    pthread_join(receiveThreadId, NULL);

    close(clientTransferState.socket);
}



void *serializeTransferData(void* src, int32_t len, enum CommandType type){
    int find=0;
    for(int i=0;i<SERVER_HANDLE_MESSAGE_TYPE_LIST_SIZE;i++){
        if(type==serverHandleMessageTypeList[i]){
            find=1;
            break;
        }
    }
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
#define COMMAND_REQUEST_LIST_SIZE   (6)
    const int command_request_list[]={FIND_REQUEST,SET_REQUEST,UPDATE_REQUEST,FIND_LESS_REQUEST,FIND_MORE_REQUEST,DELETE_REQUEST};
    for(int i=0;i<COMMAND_REQUEST_LIST_SIZE;i++){
        if(type==command_request_list[i]){
            ConsoleCommand *consoleCommand=(ConsoleCommand*)src;
            TransferCommand transferCommand;
            memset((void*)&transferCommand,0,sizeof(TransferCommand));
            if(consoleCommand->key_file){
                long size=getFileSize(consoleCommand->key);
                if(size<=0){
                    fprintf(stdout,"invalid key length:%ld\n",size);
                    return NULL;
                }
                transferCommand.keyLength=size;
            }else{
                if(consoleCommand->key!=NULL){
                    transferCommand.keyLength=strlen(consoleCommand->key);
                }
            }
            if(consoleCommand->value_file){
                long size=getFileSize(consoleCommand->value);
                if(size<=0){
                    fprintf(stdout,"invalid key length:%ld\n",size);
                    return NULL;
                }
                transferCommand.valueLength=size;
            }else{
                if(consoleCommand->value!=NULL){
                    transferCommand.valueLength=strlen(consoleCommand->value);
                }
            }
            transferCommand.totalLength=transferCommand.keyLength+transferCommand.valueLength;
            printf("transfer command:%d %d %d\n",transferCommand.totalLength,transferCommand.keyLength,transferCommand.valueLength);
            char *buffer=(char*)malloc(sizeof(transferCommand.totalLength)*3+sizeof(PacketHeader));
            PacketHeader header={
                    .dataType=type
            };
            memcpy(buffer,&header,sizeof(PacketHeader));
            memcpy(buffer+sizeof(PacketHeader),&transferCommand,sizeof(transferCommand.totalLength));
            memcpy(buffer+sizeof(PacketHeader)+sizeof(transferCommand.totalLength),&transferCommand.keyLength,sizeof(transferCommand.totalLength));
            memcpy(buffer+sizeof(PacketHeader)+sizeof(transferCommand.totalLength)*2,&transferCommand.valueLength,sizeof(transferCommand.totalLength));
            return buffer;
        }
    }

    return NULL;
}

int sendSerializeData(ClientTransferState*state,void *serializeData,int32_t beforeSerializeLength){
    return sendRawData(state,serializeData,beforeSerializeLength+sizeof(PacketHeader));
}

int sendFileData(ClientTransferState*state,const char* filename){
    FILE* fp = fopen(filename, "rb");
//    fseek(fp, 0, SEEK_END);
//    long size = ftell(fp);
//    fclose(fp);

//    fp= fopen(filename, "rb");
    char buffer[SOCKET_BUFFER_SIZE];
    size_t bytesRead;
    size_t totalBytesSent = 0;
    size_t sendResult;

    while ((bytesRead = fread(buffer, 1, SOCKET_BUFFER_SIZE, fp)) > 0) {
        sendResult = send(state->socket,buffer, bytesRead,0);
        totalBytesSent += sendResult;
        if(sendResult<bytesRead){
            fprintf(stdout,"Not all data sent. Sent %zu out of %zu bytes.",sendResult,bytesRead);
            state->connect=0;
            fprintf(stdout,"invalid data transfer,abort connection.");
            return -1;
        }
    }
    printf("send file bytes:%zu\n",totalBytesSent);

    return totalBytesSent;
}

int sendRawData(ClientTransferState*state,const char*buffer,int32_t len){
    size_t bytesRead=0;
    size_t totalBytesSent = 0;
    size_t sendResult;

    size_t currentIndex=0;
    bytesRead=(len)>SOCKET_BUFFER_SIZE
              ?SOCKET_BUFFER_SIZE:len;


    while(bytesRead!=0){
        sendResult=send(state->socket,buffer+currentIndex,bytesRead,0);
        totalBytesSent+=sendResult;
        if(sendResult<bytesRead){
            fprintf(stdout,"Not all data sent. Sent %zu out of %zu bytes.",sendResult,bytesRead);
            state->connect=0;
            fprintf(stdout,"invalid data transfer,abort connection.");
            return -1;
        }
        currentIndex+=sendResult;
        bytesRead=(currentIndex+SOCKET_BUFFER_SIZE)<(len)
                  ?SOCKET_BUFFER_SIZE:(len-currentIndex);
    }
    printf("send bytes count:%zu\n",totalBytesSent);

    return totalBytesSent;
}

PacketPayload deserializeActualData(ClientTransferState*state,void* header, int32_t len){
    char *str=header;
    for(int i=0;i<len;i++){
        printf("%d ",str[i]);
    }
    printf("\n");

    PacketHeader packetHeader=*(PacketHeader*)header;
    PacketPayload ret={
            NULL,0,-1
    };
    int find=0;
    for(int i=0;i<SERVER_RETURN_MESSAGE_TYPE_LIST_SIZE;i++){
        if(packetHeader.dataType==serverReturnMessageTypeList[i]){
            find=1;
            break;
        }
    }
    if(!find){
        printf("invalid data type,abort resolve");
        return ret;
    }
    for(int i=0;i<IMMUTABLE_LIST_SIZE;i++){
        if(packetHeader.dataType==immutableList[i]){
            ret.dataType=packetHeader.dataType;
            ret.actualLen=len-sizeof(PacketHeader);
            ret.src=(header+sizeof(PacketHeader));
            return ret;
        }
    }
    return ret;

}
