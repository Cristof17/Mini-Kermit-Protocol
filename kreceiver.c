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
	uint16_t crc;
	packet p;
	packet ack_p;
	s_packet s;
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
		print_status(retransmitted);
	}
	if (retransmitted == 4){
		//stop connection
		goto RELEASE;
	} else {
		memset(&p, 0, sizeof(p));
		memcpy(&p, y->payload, sizeof(p));
		//show_packet(p);
		memset(&ack_p, 0, sizeof(ack_p));
		crc_calculat = crc16_ccitt(&p, sizeof(packet)- 4);
		crc_primit = p.check;
		printf("CRC calculat = %d, primit = %d\n", crc_calculat, crc_primit);
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
			printf("Am primit S cu erori\n");
		} else {
			ack_p.type = Y;
			printf("Am primit S fără erori\n");
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
			print_status(retransmitted);
		}
		if (retransmitted == 4){
			//stop connection
			goto RELEASE;
		} else {
			memset(&p, 0, sizeof(p));
			memcpy(&p, y->payload, sizeof(p));
			printf("[%s]: Am primit %d\n",__FILE__, p.seq);
			//show_packet(p);
			crc_calculat = crc16_ccitt(&p, sizeof(packet)-4);
			crc_primit = p.check;
			printf("[%s]: CRC calculat = %d, CRC primit = %d pentru %d\n", __FILE__, crc_calculat, crc_primit, p.seq);
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
				printf("[%s]: Am primit fara erori 2 %d\n", __FILE__,p.seq);
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
			//show_packet(p);	
			switch(type){
				case S:{
					printf("[%s]: Am primit un mesaj de tip S\n", __FILE__);
					break;
				}
				case F:{
					printf("[%s]: Am primit un mesaj de tip F\n", __FILE__);
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
					printf("[%s]: Am primit un mesaj de tip D\n", __FILE__);
					printf("[%s]: Dimensiunea fisierului = %d\n", __FILE__, p.len);
					if (f != NULL){
						int bytesWritten = fwrite(p.data, 1, p.len, f);
						printf("[%s]: Fwrite errno = %d\n", __FILE__, errno);
						fflush(f);
						printf("[%s]: Am scris în fisier %d\n", __FILE__, bytesWritten);
					} else {
						printf("[%s]: File is null\n");
						goto RELEASE;
					}
					break;
				}
				case Z:{
					printf("[%s]: Am primit un mesaj de tip Z\n", __FILE__);
					if (f != NULL){
						fflush(f);
						fclose(f);
						printf("[%s]: Am închis fisierul\n");
						f = NULL;
					}
					break;
				}
				case B:{
					printf("[%s]: Am primit B\n", __FILE__);
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
			printf("[%s]: Ajung aici \n", __FILE__);
			fflush(f);
			fclose(f);
		}
		printf("[%s]: Ajung aici \n", __FILE__);
	return 0;
}
