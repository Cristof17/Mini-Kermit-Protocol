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
	int seq = 0;
	uint16_t crc;
	packet p;
	s_packet s;
	FILE *file;
	msg *y;
	int retransmitted = 0;
	int crc_calculat = 0;
	int crc_primit = 0;
	int flag = 0;

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
	while ((y = receive_message_timeout(TIME * 1000)) == NULL
		&& retransmitted < 4){
		retransmitted++;
		continue;
	}
	if (retransmitted == 4){
		//stop connection
		//goto release
	} else {
		memset(&p, 0, sizeof(p));
		memcpy(&p, y->payload, sizeof(p));
		printf("received crc = %d\n", p.check);
		show_packet(p);
		crc_calculat = crc16_ccitt(&p, sizeof(packet)- 4);
		crc_primit = p.check;
		printf("CRC calculat = %d, primit = %d\n", crc_calculat, crc_primit);
		memset(&r, 0, sizeof(r));
		memcpy(&r, y, sizeof(r));
		memset(&t, 0, sizeof(msg));
		p.soh = SOH;
		p.len = 0;
		p.mark = MARK;
		seq = p.seq;
		seq++;
		p.seq = seq;
		if (crc_calculat != crc_primit){
			p.type = N;
			printf("Am primit S cu erori\n");
		} else {
			p.type = Y;
			printf("Am primit S fără erori\n");
		}
		memcpy(&t.payload, &p, sizeof(p));
		t.len = sizeof(p);
		rc = send_message(&t);
		DIE (rc < 0, "Cannot send ACK for parameter packet");
		retransmitted = 0;
	}

	printf("AIci \n");

	//process S packet

	/*
	while(1){
		while ((y = receive_message_timeout(TIME * 1000)) == NULL
			&& retransmitted < 4){
			retransmitted++;
			send_message(&t);
			printf("[%s]: Tot trimit cu retransmit = %d\n", __FILE__, retransmitted);
			continue;
		}
		if (retransmitted == 4){
			//stop connection
			//goto release
		} else {
			memset(&p, 0, sizeof(p));
			memcpy(&p, y->payload, sizeof(p));
			crc_calculat = crc16_ccitt(&p, sizeof(packet)-4);
			printf("CRC pentru S în receiver este %d\n", crc_calculat); 
			crc_primit = p.check;
			memset(&r, 0, sizeof(r));
			memcpy(&r, y, sizeof(r));
			memset(&t, 0, sizeof(msg));
			p.soh = SOH;
			p.len = 0;
			p.mark = MARK;
			seq = p.seq;
			seq++;
			p.seq = seq;
			if (crc_calculat != crc_primit){
				p.type = N;
			} else {
				p.type = Y;
			}
			memcpy(&t.payload, &p, sizeof(p));
			t.len = sizeof(p);
			rc = send_message(&t);
			DIE (rc < 0, "Cannot send ACK for parameter packet");
			retransmitted = 0;
			printf("Tot intru in else\n");
			
			if (crc_calculat != crc_primit){
				continue;
			}
			//luare actiune pentru fiecare tip de mesaj
			show_packet(p);	
			switch(p.type){
				case S:{
					break;
				}
				case F:{
					break;
				}
				case D:{
					break;
				}
				case Z:{
					break;
				}
				case B:{
					flag = 10;
					break;
				}
			}
			if (flag == 10)
				break;
		}
	}
	*/

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
