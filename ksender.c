#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"
#include "helpers.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char** argv) {
    msg t;
	msg *curr;
	int i = 0; //file index
	int rc = 0; //return code
	int seq = 0; //current_packet_index
	uint16_t crc = 0;
	//TODO Save current packet and current index

    init(HOST, PORT);

    sprintf(t.payload, "Hello World of PC");
    t.len = strlen(t.payload);
    send_message(&t);

    msg *y = receive_message_timeout(5000);
    if (y == NULL) {
        perror("receive error");
		//Resend
    } else {
        printf("[%s] Got reply with payload: %s\n", argv[0], y->payload);
    }

	//TODO read arguments
    if (argc < 2){
		printf("[%s] Too few arguments\n", argv[0]);
		return -1;
	}

	//TODO send initial sequence
	s_packet s;  
	packet p;
	memset(&s, 0, sizeof(s_packet));
	memset(&p, 0, sizeof(packet));
	uint8_t maxl = 250;
	uint8_t time = 5;
	uint8_t npad = 0x00;
	uint8_t padc = 0x00;
	uint8_t eol = 0x0D;
	memcpy(&s.maxl, &maxl, sizeof(maxl));
	memcpy(&s.time, &time, sizeof(time));
	memcpy(&s.npad, &npad, sizeof(npad));
	memcpy(&s.padc, &padc, sizeof(padc));
	memcpy(&s.eol, &eol, sizeof(eol));
	memset(&s.qctl, 0, sizeof(s.qctl));
	memset(&s.qbin, 0, sizeof(s.qbin));
	memset(&s.chkt, 0, sizeof(s.chkt));
	memset(&s.rept, 0, sizeof(s.rept));
	memset(&s.capa, 0, sizeof(s.capa));
	memset(&t, 0, sizeof(msg));
	memset(&p, 0, sizeof(packet));
	uint8_t soh = 0x01;
	uint8_t len = 1 + 1 + 11 + 2 + 1;
	//len - 1 means the length of the message minus MARK
	p.soh = soh;
	p.len = len;
	p.seq = seq;
	p.type = S;
	crc= crc16_ccitt(&p, p.len - 3);
	p.check = crc;
	//the size of the status fields is the len + the first two fields
	memcpy(&p.data, &s, len +2);
	memcpy(&t.payload, &p, sizeof(p));
	//SOH + LEN + Len
	t.len = sizeof(p);

	//CRC for S
	//Send s
	rc = send_message(&t);
	DIE(rc < 0, "Cannot send message S");
	
	show_packet(p);
	print_crc(p);

	//TODO Send init
	for (i = 1; i < argc-1; ++i){
		char *filename = argv[i];
		FILE * f = fopen(filename, "r+");
		DIE(f == NULL, "Filename is null ");

		fclose(f);
	}

	//TODO split files

    return 0;
}
