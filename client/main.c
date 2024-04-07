#include"transfer.h"
#include"event_hander.h"
#include"request_processor.h"

void testTransferWithOneByte(){
    initEventHandler();
    initRequestProcessor();
    ClientTransferState state;
    memset(&state,0,sizeof(ClientTransferState));

    const char* queryKey="query_key_test";

    for(int i=0;i<strlen(queryKey);i++){
        printf("%u ",queryKey[i]);
    }
    printf("\n");


    QueryKeyRequest request={
            strlen(queryKey),queryKey
    };
    int len=sizeof(PacketHeader)+sizeof(request.keyLength)+request.keyLength;
    char *data=serializeTransferData(&request,0,QUERY_KEY_REQUEST);
    printf("expected len:%d\n",len);
    for(int i=0;i<len;i++){
        printf("%u ",data[i]);
    }
    printf("\n");
    for(int i=0;i<len;i++){
        handle_message(data+i,0,1,&state);
    }
}

void testTransferWithMultiBytes(){
    initEventHandler();
    initRequestProcessor();
    ClientTransferState state;
    memset(&state,0,sizeof(ClientTransferState));

    const char* queryKey="query_key_test";

    for(int i=0;i<strlen(queryKey);i++){
        printf("%u ",queryKey[i]);
    }
    printf("\n");

    QueryKeyRequest request={
            strlen(queryKey),queryKey
    };
    int len=sizeof(PacketHeader)+sizeof(request.keyLength)+request.keyLength;
    char *data=serializeTransferData(&request,0,QUERY_KEY_REQUEST);
    printf("expected len:%d\n",len);    //22

    for(int i=0;i<3;i++){
        handle_message(data,0,1,&state);
        handle_message(data+1,0,3,&state);
        handle_message(data+4,0,9,&state);
        handle_message(data+13,0,7,&state);
        handle_message(data+20,0,2,&state);
    }
}

void testTransferWithMultiPacket(){
    initEventHandler();
    initRequestProcessor();
    ClientTransferState state;
    memset(&state,0,sizeof(ClientTransferState));

    const char* queryKey="query_key_test";

    for(int i=0;i<strlen(queryKey);i++){
        printf("%u ",queryKey[i]);
    }
    printf("\n");


    QueryKeyRequest request={
            strlen(queryKey),queryKey
    };
    int len=sizeof(PacketHeader)+sizeof(request.keyLength)+request.keyLength;
    char *data=serializeTransferData(&request,0,QUERY_KEY_REQUEST);
    printf("expected len:%d\n",len);
    for(int i=0;i<len;i++){
        printf("%u ",data[i]);
    }
    printf("\n");
    for(int i=0;i<len;i++){
        handle_message(data+i,0,1,&state);
    }

    const char* queryValue="query_key_test_finish,this is value";


    for(int i=0;i<strlen(queryValue);i++){
        printf("%u ",queryValue[i]);
    }
    printf("\n");


    QueryKeyResult result={
            strlen(queryValue),queryValue
    };
    int valueLen=sizeof(PacketHeader)+sizeof(result.valueLength)+result.valueLength;
    char *valueData=serializeTransferData(&result,0,QUERY_KEY_RESULT);
    printf("expected len:%d\n",valueLen);
    for(int i=0;i<valueLen;i++){
        printf("%u ",valueData[i]);
    }
    printf("\n");
    for(int i=0;i<valueLen;i++){
        handle_message(valueData+i,0,1,&state);
    }
}

//#define LOCAL_TEST

int main() {

#ifdef  LOCAL_TEST
    testTransferWithOneByte();
    testTransferWithMultiBytes();
    testTransferWithMultiPacket();
#else
    initEventHandler();
    initRequestProcessor();
    initClient();
#endif
}
