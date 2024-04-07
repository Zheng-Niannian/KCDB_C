#include"transfer.h"
#include"event_hander.h"
#include"request_processor.h"

int main() {
    initEventHandler();
    initRequestProcessor();
    initServer();
}
