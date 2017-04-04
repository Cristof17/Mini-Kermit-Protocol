#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char** argv) {
    msg t;


    init(HOST, PORT);


    sprintf(t.payload, "Hello World of PC");
    t.len = strlen(t.payload);
    send_message(&t);


    msg *y = receive_message_timeout(5000);
    if (y == NULL) {
        perror("receive error");
    } else {
        printf("[%s] Got reply with payload: %s\n", argv[0], y->payload);
    }

    
    return 0;
}
