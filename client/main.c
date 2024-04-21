#include"transfer.h"
#include"event_handler.h"
#include"request_processor.h"
#include"logger.h"

//#define LOCAL_TEST

int main() {

#ifdef  LOCAL_TEST

#else
    init_logger();
    initEventHandler();
    initRequestProcessor();
    initClient();
#endif
}
