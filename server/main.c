#include"transfer.h"
#include"event_hander.h"
#include"request_processor.h"
#include"logger.h"

int main() {
    init_logger();
    initEventHandler();
    initRequestProcessor();
    initServer();
}
