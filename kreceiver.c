//Rotsching Cristofor 343C1

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"
#include "helpers.h"
#include <errno.h>
#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
    msg r, t;
	int rc = 0;
	int seq = 0;
	packet p;
	packet ack_p;
	FILE *f = NULL;
	msg *y;
	int retransmitted = 0;
	int crc_calculat = 0;
	int crc_primit = 0;
	int flag = 0;
	int type = 0;

    init(HOST, PORT);
	
	while ((y = receive_message_timeout(TIME * 1000)) == NULL
		&& retransmitted < 4){
		retransmitted++;
	}
	if (retransmitted == 4){
		goto RELEASE;
	} else {
		memset(&p, 0, sizeof(p));
		memcpy(&p, y->payload, sizeof(p));
		memset(&ack_p, 0, sizeof(ack_p));
		crc_calculat = crc16_ccitt(&p, sizeof(packet)- 4);
		crc_primit = p.check;
		memset(&r, 0, sizeof(r));
		memcpy(&r, y, sizeof(r));
		memset(&t, 0, sizeof(msg));
		memset(&ack_p, 0, sizeof(packet));
		ack_p.soh = SOH;
		ack_p.len = 0;
		ack_p.mark = MARK;
		seq = p.seq;
		seq++;
		ack_p.seq = seq;
		if (crc_calculat != crc_primit){
			ack_p.type = N;
		} else {
			ack_p.type = Y;
		}
		memcpy(&t.payload, &ack_p, sizeof(p));
		t.len = sizeof(ack_p);
		rc = send_message(&t);
		DIE (rc < 0, "Cannot send ACK for parameter packet");
		retransmitted = 0;
	}

	while(1){
		while ((y = receive_message_timeout(TIME * 1000)) == NULL
			&& retransmitted < 4){
			retransmitted++;
			send_message(&t);
		}
		if (retransmitted == 4){
			goto RELEASE;
		} else {
			memset(&p, 0, sizeof(p));
			memcpy(&p, y->payload, sizeof(p));
			crc_calculat = crc16_ccitt(&p, sizeof(packet)-4);
			crc_primit = p.check;
			memset(&r, 0, sizeof(r));
			memcpy(&r, y, sizeof(r));
			memset(&t, 0, sizeof(msg));
			memset(&ack_p, 0, sizeof(ack_p));
			ack_p.soh = SOH;
			ack_p.len = 0;
			ack_p.mark = MARK;
			seq = p.seq;
			seq++;
			ack_p.seq = seq;
			if (crc_calculat != crc_primit){
				ack_p.type = N;
			} else {
				type = p.type; //save the received type and reuse packet struct
				ack_p.type = Y;
			}
			memcpy(&t.payload, &ack_p, sizeof(p));
			t.len = sizeof(p);
			rc = send_message(&t);
			DIE (rc < 0, "Cannot send ACK for parameter packet");
			retransmitted = 0;
			
			if (crc_calculat != crc_primit){
				continue;
			}
			//luare actiune pentru fiecare tip de mesaj
			switch(type){
				case S:{
					break;
				}
				case F:{
					if (f == NULL){
						char filename[MAXL];
						memset(filename, 0, MAXL);
						sprintf(filename, "recv_");
						strcat(filename, p.data);
						f = fopen(filename, "wa+");
					}
					break;
				}
				case D:{
					if (f != NULL){
						fwrite(p.data, 1, p.len, f);
						fflush(f);
					} else {
						goto RELEASE;
					}
					break;
				}
				case Z:{
					if (f != NULL){
						fflush(f);
						fclose(f);
						f = NULL;
					}
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

	RELEASE:
		if (f != NULL){
			fflush(f);
			fclose(f);
		}
	return 0;
}
