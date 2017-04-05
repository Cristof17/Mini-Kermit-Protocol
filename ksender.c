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
	int seq_asteptat = 0 ; //current_packet_index
	int seq_trimis = 0;
	uint16_t crc = 0;
	int retransmitted =0;
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
	uint8_t maxl = MAXL;
	uint8_t time = TIME;
	uint8_t npad = NPAD;
	uint8_t padc = PADC;
	uint8_t eol = EOL;
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
	uint8_t soh = SOH;
	uint8_t len = 1 + 1 + 11 + 2 + 1;
	//len - 1 means the length of the message minus MARK
	p.soh = soh;
	p.len = len;
	p.seq = seq_trimis;
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
	seq_trimis++;
	
	show_packet(p);
	print_crc(p);

	//wait for ack for parameter packet
	memset(y, 0, sizeof(msg));
	//conver to milies
    y = receive_message_timeout(TIME * 1000);
    if (y == NULL) {
        perror("receive error");
		//here is a problem
		//resend last packet;
		rc = send_message(&t);
		DIE(rc < 0, "Cannot resend send message S");
		retransmitted++;
		//while has to receive packets
		while (((y = receive_message_timeout(TIME * 1000)) == NULL)
				&& retransmitted < 3){
				rc = send_message(&t);
				DIE(rc < 0, "Cannot resend send message S");
				retransmitted++;
		}
		if (retransmitted == 4){
			//TODO Stop transmission
			//stop connection
		} else{
			seq_asteptat++;
		}
    } else { 
		memset(&p, 0, sizeof(packet));
		memcpy(&p, y->payload, y->len);
		if (p.type == N){
			//resend last packet with seq increased
			memset(&t, 0, sizeof(msg));
			p.seq = seq_trimis;
			rc = send_message(&t);
			DIE(rc < 0, "Cannot send NACK");
		}else {
			//increase seq
			seq_asteptat++;
		}
        printf("[%s] Got  %s of type %d\n", argv[0], y->payload, p.type);
    }

	//TODO Send init
	for (i = 1; i < argc-1; ++i){
		char *filename = argv[i];
		char *strip = NULL;
		FILE *f = fopen(filename, "r+");
		DIE(f == NULL, "Filename is null ");
		int filename_len = strlen(filename);

		//create F packet
		memset(&p, 0, sizeof(packet));
		memset(&t, 0, sizeof(msg));
		p.soh = SOH;
		p.len = 1 + 1 + filename_len + 2 + 1;
		p.seq = seq_trimis;
		p.type = F;
		strcpy(p.data, filename);
		crc = crc16_ccitt(&p, 4 + filename_len);
		p.check = crc;
		show_packet(p);
		
		//put it into a message
		
		//SOH + LEN + SEQ + TYPE +_DATA + CHECK + MARK
		t.len = (4 + filename_len + 3) + 1;
		//plus the \0 of the string
		memcpy(t.payload, &p, sizeof(p));
		rc = send_message(&t);
		print_message(t);
		DIE (rc < 0, "Cannot send message with filename");
		y = receive_message_timeout(TIME * 1000);
		if (y == NULL){
			//resend the next packet
			rc = send_message(&t);
			DIE (rc < 0, "Cannot resend lost data packet");
		} else {
			memset(&p, 0, sizeof(p));
			memcpy(&p, y->payload, sizeof(p));
			show_packet(p);
			if (p.type == Y){
				seq_asteptat++;
			} else {
				rc = send_message(&t);
				DIE(rc < 0, "Cannot resend corrupted file");
			}
		}
		fclose(f);
	}

	//TODO split files

    return 0;
}
