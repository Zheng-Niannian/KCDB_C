#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "transfer.h"

int server_socket;
struct sockaddr_in server_address, accept_address;
int32_t nextClientId = 1;

void afterHandleMessage(ClientTransferState *state) {
    state->messageType = NOT_WAIT;
    state->unknownMessageLength = 0;
    state->unknownMessageType = 0;
    state->messageLength = 0;
    state->messageId = 0;
    state->receivedLength = 0;
    state->busy = 0;
    if (state->buffer) {
        free(state->buffer);
        state->buffer = NULL;
    }
    if (state->fp) {
        fclose(state->fp);
        state->fp = NULL;
    }
}

extern void handleMessage(ClientTransferState *state, PacketPayload *payload);

const enum CommandType immutableList[] = {
        NOT_WAIT, LOGIN_REQUEST, LOGIN_RESULT,SAVE_REQUEST,SAVE_RESULT
};

const int immutableTransferTypeLength[] = {
        0, sizeof(LoginRequest), sizeof(LoginResult),sizeof(SaveRequest),sizeof(SaveResult)
};
#define IMMUTABLE_LIST_SIZE     (5)

const enum CommandType serverHandleMessageTypeList[] = {
        LOGIN_REQUEST, FIND_REQUEST, SET_REQUEST, UPDATE_REQUEST, FIND_LESS_REQUEST, FIND_MORE_REQUEST, DELETE_REQUEST,SAVE_REQUEST
};
#define SERVER_HANDLE_MESSAGE_TYPE_LIST_SIZE    (8)

const enum CommandType serverReturnMessageTypeList[] = {
        LOGIN_RESULT, FIND_RESULT, SET_RESULT, UPDATE_RESULT, FIND_LESS_RESULT, FIND_MORE_RESULT, DELETE_RESULT,SAVE_RESULT
};
#define SERVER_RETURN_MESSAGE_TYPE_LIST_SIZE     (8)

const enum CommandType transferCommandReturnTypeList[]={
        FIND_RESULT,SET_RESULT,UPDATE_RESULT,FIND_LESS_RESULT,FIND_MORE_RESULT,DELETE_RESULT
};
#define TRANSFER_COMMAND_RETURN_TYPE_LIST_SIZE  (6)


void handle_message(char *data, int socket, int32_t count, ClientTransferState *state) {
    char *handleDataAwait = NULL;
    int handleDataAwaitCount = 0;

    printf("state.messageType:%d\n", state->messageType);

#pragma region RESOLVE_MESSAGE_TYPE
    if (state->messageType == NOT_WAIT) {
        if (state->buffer == NULL && count < sizeof(PacketHeader)) {
            state->unknownMessageType = 1;
            state->buffer = malloc(sizeof(PacketHeader));
            memset(state->buffer, 0, sizeof(PacketHeader));
            memcpy(state->buffer, data, count);
            state->receivedLength = count;
            fprintf(stdout, "find small piece,size less than PacketHeader,copy %u bytes\n", count);
            return;
        }
        if (state->buffer != NULL && count < sizeof(PacketHeader)) {
            if (state->receivedLength + count >= sizeof(PacketHeader)) {

                memcpy(state->buffer + state->receivedLength, data, sizeof(PacketHeader) - state->receivedLength);
                PacketHeader *header = (PacketHeader *) state->buffer;
                state->messageType = header->dataType;
                fprintf(stdout, "find small piece,construct header success,message type:%u\n", state->messageType);
                state->unknownMessageType = 0;
                char *previousBuffer = state->buffer;
                state->buffer = malloc(sizeof(PacketHeader) * 2);
                memcpy(state->buffer, previousBuffer, sizeof(PacketHeader));
                free(previousBuffer);

                int useCount = sizeof(PacketHeader) - state->receivedLength;
                state->receivedLength = sizeof(PacketHeader);
                data += useCount;
                count -= useCount;
                state->unknownMessageLength = 1;

//                for(int i=0;i<IMMUTABLE_LIST_SIZE;i++){
//                    if(state->messageType==immutableList[i]){
//                        state->unknownMessageLength=0;
//                        state->messageLength=immutableList[i];
//                    }
//                }

            } else {
                fprintf(stdout, "find small piece,construct header not finish\n");
                memcpy(state->buffer + state->receivedLength, data, count);
                state->receivedLength += count;
                return;
            }
        }
    }
#pragma endregion


    if (state->messageType != NOT_WAIT) {
        int previousStateReceivedCount = state->receivedLength;
        state->receivedLength += count;
        printf("[INFO]Received stream data, count:%d, totalCount:%d, totalLength:%d\n", count, state->receivedLength,
               state->messageLength);

        if (state->unknownMessageLength) {
            if (state->receivedLength >= sizeof(PacketHeader) * 2) {
                memcpy(state->buffer + previousStateReceivedCount, data,
                       sizeof(PacketHeader) * 2 - previousStateReceivedCount);
                PacketHeader *payloadStart = (PacketHeader *) (state->buffer + sizeof(PacketHeader));
                state->messageLength = sizeof(PacketHeader) * 2 + payloadStart->dataType;
                fprintf(stdout, "recalculate message length finish,message length:%u\n", state->messageLength);
                state->unknownMessageLength = 0;
                char *previousBuffer = state->buffer;

                if (state->messageLength > TRANSFER_MEMORY_BUFFER_MAX_SIZE) {
                    sprintf(state->filename_buffer, "/home/KVF/zheng1/tmp/file_buffer_%d_%d.bin", state->clientId,
                            state->messageId++);
                    fprintf(stdout, "try to create file: %s\n", state->filename_buffer);
                    state->fp = fopen(state->filename_buffer, "w+");
                    if (!state->fp) {
                        fprintf(stdout, "cannot create file buffer for client%d,abort connection\n", state->clientId);
                        state->connect = 0;
                        return;
                    }
                    fwrite(state->buffer, 1, sizeof(PacketHeader) * 2, state->fp);
                    state->buffer = NULL;
                } else {
                    state->buffer = malloc(state->messageLength);
                    memset(state->buffer, 0, state->messageLength);
                    memcpy(state->buffer, previousBuffer, sizeof(PacketHeader) * 2);
                }

                free(previousBuffer);

                int useCount = sizeof(PacketHeader) * 2 - previousStateReceivedCount;
                count -= useCount;
                data += useCount;
                printf("use count:%d\n", useCount);


            } else {
                memcpy(state->buffer + previousStateReceivedCount, data, count);
                fprintf(stdout, "cannot recalculate message length\n");
                return;
            }
        }

        if (state->receivedLength > state->messageLength) {
            int allowance = state->messageLength - (state->receivedLength - count);
            if (state->buffer) {
                memcpy(state->buffer + state->receivedLength - count, data, allowance);
            } else {
                fwrite(data, 1, allowance, state->fp);
            }

            int32_t remain_data = state->receivedLength - state->messageLength;
            state->receivedLength = state->messageLength;
            printf("current packet receive finish, remain data count:%d\n", remain_data);
            int nextPackageDataIndex = count - remain_data;
            handleDataAwait = (char *) malloc(sizeof(char) * remain_data);
            memcpy(handleDataAwait, data + nextPackageDataIndex, remain_data);
            handleDataAwaitCount = remain_data;
        } else {
            int writeIndex = state->receivedLength - count;
            printf("write index:%d\n", writeIndex);

            if (state->buffer) {
                memcpy(state->buffer + state->receivedLength - count, data, count);
            } else {
                fwrite(data, 1, count, state->fp);
            }
        }
        if (state->receivedLength == state->messageLength) {
            printf("[SUCCESS]Receive stream data completed\n");
            printf("in handle message,before deserialize,message length=%d\n", state->messageLength);


            PacketPayload payload = deserializeActualData(state, state->buffer, state->messageLength);
            state->messageType = NOT_WAIT;
            printf("in handle message,after deserialize,message length=%d\n", state->messageLength);
            handleMessage(state, &payload);

            afterHandleMessage(state);

        }
    } else {
        PacketHeader *header = (PacketHeader *) data;

        printf("check immutable_list\n");
        int process = 0;
        for (int i = 0; i < IMMUTABLE_LIST_SIZE; i++) {
            if (header->dataType == immutableList[i]) {
  
                int needPayloadBytesCount = immutableTransferTypeLength[i] - (count - sizeof(PacketHeader));
                if (needPayloadBytesCount <= 0) {
                    PacketPayload payload = deserializeActualData(state, data, count);
                    process = 1;
                    handleMessage(state, &payload);
                    afterHandleMessage(state);
                } else {
                    state->messageType = header->dataType;
                    state->buffer = malloc(immutableTransferTypeLength[i] + sizeof(PacketHeader));
                    state->messageLength = immutableTransferTypeLength[i] + sizeof(PacketHeader);
                    state->receivedLength = count;
                    memcpy(state->buffer, data, state->receivedLength);
                
                }
                int allowance = count - immutableTransferTypeLength[i] - sizeof(PacketHeader);
                if (allowance > 0) {
                    handleDataAwait = (char *) malloc(sizeof(char) * allowance);
                    memcpy(handleDataAwait, data + sizeof(PacketHeader) + immutableTransferTypeLength[i], allowance);
                    handleDataAwaitCount = allowance;
                }
            }
        }
        if (!process) {
            log_info("header.dataType:%d\n", header->dataType);
            for (int i = 1; i < SERVER_HANDLE_MESSAGE_TYPE_LIST_SIZE; i++) {
                if (header->dataType == serverHandleMessageTypeList[i]) {
                    if (count - sizeof(PacketHeader) >= 4) {
                        PacketHeader *start = (PacketHeader *) (data + sizeof(PacketHeader));
                        int32_t totalLength = start->dataType;
                        log_info("total length:%d\n", totalLength);
                        if ((totalLength + sizeof(PacketHeader) * 4) <= count) {
                            PacketPayload payload = deserializeActualData(state, data, count);
                            handleMessage(state, &payload);
                            afterHandleMessage(state);
                            int allowance = count - totalLength - sizeof(PacketHeader) * 4;
                            if (allowance > 0) {
                                handleDataAwait = (char *) malloc(sizeof(char) * allowance);
                                memcpy(handleDataAwait, data + sizeof(PacketHeader) * 4 + totalLength, allowance);
                                handleDataAwaitCount = allowance;
                            }
                        } else {
                            state->messageType = header->dataType;
                            state->messageLength = sizeof(PacketHeader) * 4 + totalLength;
                            state->receivedLength = count;

                            if (state->messageLength > TRANSFER_MEMORY_BUFFER_MAX_SIZE) {
                                sprintf(state->filename_buffer, "/home/zheng1/tmpfile_buffer_%d_%d.bin",
                                        state->clientId, state->messageId++);
                                fprintf(stdout, "try to create file: %s\n", state->filename_buffer);
                                state->fp = fopen(state->filename_buffer, "w+");

                                if (!state->fp) {
                                    fprintf(stdout, "cannot create file buffer for client%d,abort connection\n",
                                            state->clientId);
                                    state->connect = 0;
                                    return;
                                }
                                fwrite(data, 1, state->receivedLength, state->fp);
                                free(state->buffer);
                                state->buffer = NULL;
                            } else {
                                state->buffer = malloc(state->messageLength);
                                memset(state->buffer, 0, state->messageLength);
                                memcpy(state->buffer, data, state->receivedLength);
                            }
                        }
                    } else {
                        state->messageType = header->dataType;
                        state->unknownMessageLength = 1;
                        state->buffer = malloc(sizeof(PacketHeader) * 2);
                        memset(state->buffer, 0, sizeof(PacketHeader) * 2);
                        memcpy(state->buffer, data, count);
                    }

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
    char buffer[1024] = {0};
    ClientTransferState state = *(ClientTransferState *) arg;
    free(arg);
    while (state.connect) {
        int len = recv(state.socket, buffer, SOCKET_BUFFER_SIZE, 0);
        if (len > 0) {
            printf("receive message length: %d\n", len);
            handle_message(buffer, state.socket, len, &state);
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
    if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    while (1) {
        char buffer[SOCKET_BUFFER_SIZE] = {0};

        int newClient = accept(server_socket, (struct sockaddr *) &accept_address, &len);
        int receive = recv(newClient, buffer, SOCKET_BUFFER_SIZE, 0);
        if (receive <= 0) {
            perror("recv failed");
            continue;
        }

        ClientTransferState *state = (char *) malloc(sizeof(ClientTransferState));
        if (!state) {
            fprintf(stdout, "allocate memory failed,abort connection\n");
            close(newClient);
            continue;
        }
        memset(state, 0, sizeof(ClientTransferState));
        state->socket = newClient;
        state->clientId = 0;

        PacketPayload payload = deserializeActualData(state, buffer, receive);
        if (!payload.src) {
            fprintf(stdout, "unable to deserialize login request\n");
            close(newClient);
            continue;
        }

        LoginRequest loginRequest = *(LoginRequest *) payload.src;

        LoginResult result = {
                0, 0
        };

        if (strcmp(loginRequest.username, loginRequest.password) == 0) {
            result.loginResult = 1;
            result.clientId = nextClientId++;
        }

        state->clientId = result.clientId;

        fprintf(stdout, "login result:%u, clientId:%u\n", result.loginResult, result.clientId);

        char *retBuffer = serializeTransferData(&result, sizeof(LoginResult), LOGIN_RESULT);
        if (!retBuffer) {
            fprintf(stdout, "serialize login result failed,abort connection\n");
            close(newClient);
            continue;
        }


        int sendData = sendSerializeData(state, retBuffer, sizeof(LoginResult));
        free(retBuffer);
        if (sendData <= 0) {
            fprintf(stdout, "unable to send login result,abort connection\n");
            close(newClient);
            continue;
        }

        if (!result.loginResult) {
            fprintf(stdout, "login failed,incorrect username or password\n");
            close(newClient);
            continue;
        }

        state->connect = 1;
        pthread_create(&state->threadId, NULL, clientThread, state);
        fprintf(stdout, "establish new connection\n");

    }
}

void *serializeTransferData(void *src, int32_t len, enum CommandType type) {
    int find = 0;
    for (int i = 0; i < SERVER_RETURN_MESSAGE_TYPE_LIST_SIZE; i++) {
        if (type == serverReturnMessageTypeList[i]) {
            find = 1;
            break;
        }
    }
    if (!find) {
        fprintf(stdout, "invalid transfer data type:%d, abort serialize\n", type);
        return NULL;
    }

    for (int i = 0; i < IMMUTABLE_LIST_SIZE; i++) {
        if (type == immutableList[i]) {
            char *buffer = (char *) malloc(sizeof(char) * len + sizeof(PacketHeader));
            if (!buffer) {
                return NULL;
            }
            PacketHeader header = {
                    .dataType=type
            };
            memcpy(buffer, &header, sizeof(PacketHeader));
            memcpy(buffer + sizeof(PacketHeader), src, len);
            return buffer;
        }
    }

    for(int i=0;i<TRANSFER_COMMAND_RETURN_TYPE_LIST_SIZE;i++){
        if(type==transferCommandReturnTypeList[i]){
            TransferCommandPayload *payload=(TransferCommandPayload*)src;
            int totalLength=payload->keyLength+payload->valueLength;
            int needBufferLength=4*sizeof(PacketHeader);//data_type,total_length,key_length,value_length
            char *buffer=(char*)malloc(sizeof(char)*needBufferLength);
            if(!buffer){
                return NULL;
            }
            PacketHeader header = {
                    .dataType=type
            };
            memcpy(buffer,&header,sizeof(PacketHeader));
            header.dataType=totalLength;
            memcpy(buffer+sizeof(PacketHeader),&header,sizeof(PacketHeader));
            header.dataType=payload->keyLength;
            memcpy(buffer+sizeof(PacketHeader)*2,&header,sizeof(PacketHeader));
            header.dataType=payload->valueLength;
            memcpy(buffer+sizeof(PacketHeader)*3,&header,sizeof(PacketHeader));
            return buffer;
        }

    }
    return NULL;
}

int sendSerializeData(ClientTransferState *state, void *serializeData, int32_t beforeSerializeLength) {
    size_t bytesRead = 0;
    size_t totalBytesSent = 0;
    size_t sendResult;

    size_t currentIndex = 0;
    int actualSerializeLength = beforeSerializeLength + sizeof(PacketHeader);
    bytesRead = actualSerializeLength > SOCKET_BUFFER_SIZE
                ? SOCKET_BUFFER_SIZE : actualSerializeLength;


    while (bytesRead != 0) {
        printf("send ...\n");
        sendResult = send(state->socket, serializeData + currentIndex, bytesRead, 0);
        totalBytesSent += sendResult;
        if (sendResult < bytesRead) {
            fprintf(stdout, "Not all data sent. Sent %zu out of %zu bytes.", sendResult, bytesRead);
            state->connect = 0;
            fprintf(stdout, "invalid data transfer,abort connection.");
            return -1;
        }
        currentIndex += sendResult;
        bytesRead = (currentIndex + SOCKET_BUFFER_SIZE) < actualSerializeLength
                    ? SOCKET_BUFFER_SIZE : (actualSerializeLength - currentIndex);
    }

    fprintf(stdout, "send finish,total bytes:%zu\n", totalBytesSent);
    return totalBytesSent;
}

int sendFileData(ClientTransferState *state, const char *filename) {
    FILE *fp = fopen(filename, "rb");
//    fseek(fp, 0, SEEK_END);
//    long size = ftell(fp);
//    fclose(fp);

//    fp= fopen(filename, "rb");
    char buffer[SOCKET_BUFFER_SIZE];
    size_t bytesRead;
    size_t totalBytesSent = 0;
    size_t sendResult;

    while ((bytesRead = fread(buffer, 1, SOCKET_BUFFER_SIZE, fp)) > 0) {
        sendResult = send(state->socket, buffer, bytesRead, 0);
        totalBytesSent += sendResult;
        if (sendResult < bytesRead) {
            fprintf(stdout, "Not all data sent. Sent %zu out of %zu bytes.", sendResult, bytesRead);
            state->connect = 0;
            fprintf(stdout, "invalid data transfer,abort connection.");
            return -1;
        }
    }
    return totalBytesSent;
}

int sendRawData(ClientTransferState *state, const char *buffer, int32_t len) {
    size_t bytesRead = 0;
    size_t totalBytesSent = 0;
    size_t sendResult;

    size_t currentIndex = 0;
    bytesRead = (len) > SOCKET_BUFFER_SIZE
                ? SOCKET_BUFFER_SIZE : len;


    while (bytesRead != 0) {
        sendResult = send(state->socket, buffer + currentIndex, bytesRead, 0);
        totalBytesSent += sendResult;
        if (sendResult < bytesRead) {
            fprintf(stdout, "Not all data sent. Sent %zu out of %zu bytes.", sendResult, bytesRead);
            state->connect = 0;
            fprintf(stdout, "invalid data transfer,abort connection.");
            return -1;
        }
        currentIndex += sendResult;
        bytesRead = (currentIndex + SOCKET_BUFFER_SIZE) < (len)
                    ? SOCKET_BUFFER_SIZE : (len - currentIndex);
    }
    printf("send bytes count:%zu\n", totalBytesSent);

    return totalBytesSent;
}


PacketPayload deserializeActualData(ClientTransferState *state, void *header, int32_t len) {
    if (state->fp) {
        PacketPayload ret = {
                NULL, state->messageLength, state->messageType, state->fp
        };
        return ret;
    }
//    char *str=header;
//    for(int i=0;i<len;i++){
//        printf("%d ",str[i]);
//    }
//    printf("\n");

    PacketHeader packetHeader = *(PacketHeader *) header;
    PacketPayload ret = {
            NULL, 0, -1, NULL
    };
    int find = 0;
    for (int i = 0; i < SERVER_HANDLE_MESSAGE_TYPE_LIST_SIZE; i++) {
        if (packetHeader.dataType == serverHandleMessageTypeList[i]) {
            find = 1;
            break;
        }
    }
    if (!find) {
        printf("invalid data type,abort resolve");
        return ret;
    }
    for (int i = 0; i < IMMUTABLE_LIST_SIZE; i++) {
        if (packetHeader.dataType == immutableList[i]) {
            ret.dataType = packetHeader.dataType;
            ret.actualLen = len - sizeof(PacketHeader);
            ret.src = (header + sizeof(PacketHeader));
            return ret;
        }
    }

    for (int i = 1; i < SERVER_HANDLE_MESSAGE_TYPE_LIST_SIZE; i++) {
        if (packetHeader.dataType == serverHandleMessageTypeList[i]) {
            ret.dataType = packetHeader.dataType;
            ret.src = header + sizeof(PacketHeader);
            PacketHeader *start = (PacketHeader *) ret.src;
            ret.actualLen = start->dataType;
            return ret;
        }
    }

    return ret;

}
