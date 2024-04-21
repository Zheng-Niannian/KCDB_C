#include"request_processor.h"

//void handleQueryKeyResult(ClientTransferState *state,PacketPayload *payload){
//    QueryKeyResult *result=(QueryKeyResult*)payload->src;
//    char *resultBuffer=(char*)malloc(result->valueLength+1);
//    if(!resultBuffer){
//        fprintf(stdout,"unable to allocate memory for QueryKeyResult.value\n");
//        return;
//    }
//    memset(resultBuffer,0,result->valueLength+1);
//    char *dest= (char *) &result->valueLength;
//    memcpy(resultBuffer,dest+sizeof(result->valueLength),result->valueLength);
//    fprintf(stdout,"QueryKeyResult.value:%s\n",resultBuffer);
//    free(resultBuffer);
//}
//
//void handleQueryKeyRequest(ClientTransferState *state,PacketPayload *payload){
//    QueryKeyRequest *request=(QueryKeyRequest *)payload->src;
//    char *resultBuffer=(char*)malloc(request->keyLength + 1);
//    if(!resultBuffer){
//        fprintf(stdout,"unable to allocate memory for QueryKeyResult.value\n");
//        return;
//    }
//    memset(resultBuffer, 0, request->keyLength + 1);
//    char *dest= (char *) &request->keyLength;
//    memcpy(resultBuffer, dest+sizeof(request->keyLength), request->keyLength);
//    fprintf(stdout,"QueryKeyRequest.value:%s\n",resultBuffer);
//    free(resultBuffer);
//}

void initRequestProcessor(void){
//    setDataHandler(QUERY_KEY_RESULT,handleQueryKeyResult);
//    setDataHandler(QUERY_KEY_REQUEST,handleQueryKeyRequest);
}
