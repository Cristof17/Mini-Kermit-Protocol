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

int is_crc_ok(packet p){
	unsigned short packet_crc = p.check;
	unsigned short calculated_crc = crc16_ccitt(&p, sizeof(p) - 3);
	if (packet_crc == calculated_crc){
		return OK;
	}
	return NOT_OK;
}

int main(int argc, char** argv) {
    msg r, t;
	int rc = 0;
	int seq_asteptat = 0;
	int seq_trimis = 0;
	uint16_t crc;
	packet p;
	s_packet s;
	FILE *file;
	msg *y;

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
	
	//TODO Receive parameters
	memset(&r, 0, sizeof(msg));
	//TODO Not ok. The parameters might get lost that is why there is seg fault 
	rc = recv_message(&r);
	DIE(rc < 0, "error when receiving S packet");

	//extract parameters
	memset(&p, 0, sizeof(p));
	memset(&s, 0, sizeof(s));
	memcpy(&p, r.payload, r.len);
	memcpy(&s, p.data, p.len - 1 -1 -2 -1);
	//TODO Debug purpose
	show_packet(p);
	//check params CRC
	crc = crc16_ccitt(&p, sizeof(p) - 3); 
	print_crc(p);
	seq_trimis = p.seq;
	if (crc != p.check){
		//TODO resend status packet
		//create nack and send it
		memset(&t, 0, sizeof(t));
		create_nack(&t, seq_trimis);
	} else { 
		//create ack and send it
		memset(&t, 0, sizeof(t));
		create_ack(&t, ++seq_trimis);
	}
	print_message(t);
	send_message(&t);

	//TODO Receive the first message
	while(p.type != B){
		y = receive_message_timeout(TIME * 1000);
		if (y == NULL){
			fprintf(stderr, "TIMEOUT for seq %d\n", seq_asteptat);
			//send the last message
			send_message(&t);
		} else {
			//process message	
			print_message((*y));
			seq_asteptat++;
		}
	}

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
