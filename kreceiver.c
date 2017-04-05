#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"
#include "helpers.h"
#define HOST "127.0.0.1"
#define PORT 10001

//seq is a variable declared inside main, and thus it is passed as a parameter
void create_nack(msg *t, int seq){
	packet p;
	memset(&p, 0, sizeof(p));
	p.soh = SOH;
	p.len = 5; //all the headers after len(the data field is null)
	p.seq = seq;
	p.type = N;
	p.check = crc16_ccitt(&p, p.len);
	
	t->len = 1 + 1 + 1 + 1 + 2 + 1;
	//t.len = SOH + LEN + SEQ + TYPE + CHECK + MARK
	memcpy(t->payload, &p, 7);
}

//seq is a variable defined inside main, an needs to be passed as parameter
void create_ack(msg *t, int seq){
	packet p;
	memset(&p, 0, sizeof(p));
	p.soh = SOH;
	p.len = 5; //all the headers after len(the data field is null)
	p.seq = seq;
	p.type = Y;
	p.check = crc16_ccitt(&p, p.len);
	
	t->len = 1 + 1 + 1 + 1 + 2 + 1;
	//t.len = SOH + LEN + SEQ + TYPE + CHECK + MARK
	memcpy(t->payload, &p, 7);
}

int main(int argc, char** argv) {
    msg r, t;
	int rc = 0;
	int seq = 0;
	uint16_t crc;
	packet p;
	s_packet s;

    init(HOST, PORT);

    if (recv_message(&r) < 0) {
        perror("Receive message");
        return -1;
    }
    printf("[%s] Got msg with payload: %s\n", argv[0], r.payload);
    
    crc = crc16_ccitt(r.payload, r.len);
    sprintf(t.payload, "CRC(%s)=0x%04X", r.payload, crc);
    t.len = strlen(t.payload);
    send_message(&t);
	
	//TODO Save last received packet and index awaited

	//TODO Save current_open file

	//TODO Receive parameters
	memset(&r, 0, sizeof(msg));
	rc = recv_message(&r);
	DIE(rc < 0, "error when receiving S packet");
	memset(&p, 0, sizeof(p));
	memset(&s, 0, sizeof(s));
	memcpy(&p, r.payload, r.len);
	memcpy(&s, p.data, p.len - 1 -1 -2 -1);
	show_packet(p);
	//check params CRC
	crc = crc16_ccitt(&p, sizeof(p) - 3); 
	print_crc(p);
	if (crc != p.check){
		//TODO resend status packet
		//create nack and send it
		memset(&t, 0, sizeof(t));
		create_nack(&t, seq);
		send_message(&t);
	} else { 
		//create ack and send it
		memset(&t, 0, sizeof(t));
		create_ack(&t, seq);
		send_message(&t);
	}
	//TODO Parse parameters
	int maxl = s.maxl;
	int time = s.time;
	int npad = s.npad;
	char padc = s.padc;
	char eol = s.eol;
	print_stats(s);

	//TODO While (!received EOT)

	//TODO parse each message parameters

	//TODO receive(file_heoaders)

	//TODO create_files_on_disk

	//TODO receive(file_data)

	//TODO calculate_crc()

	//TODO Send ACK/NACK

	//TODO Write_in_file

	//TODO Close file
	return 0;
}
