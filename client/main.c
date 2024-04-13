#include"transfer.h"
#include"event_hander.h"
#include"request_processor.h"

//#define LOCAL_TEST

int main() {

#ifdef  LOCAL_TEST

#else
    initEventHandler();
    initRequestProcessor();
    initClient();
#endif
}
