#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"
#include "helpers.h"
#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
    msg r, t;
	int rc = 0;
	int seq = 0;
	uint16_t crc;
	packet p;
	s_packet s;
	FILE *f = NULL;
	msg *y;
	int retransmitted = 0;
	int crc_calculat = 0;
	int crc_primit = 0;
	int flag = 0;
	int type = 0;

    init(HOST, PORT);
	
	//TODO Not ok. The parameters might get lost that is why there is seg fault 
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
		show_packet(p);
		crc_calculat = crc16_ccitt(&p, sizeof(packet)- 4);
		crc_primit = p.check;
		printf("CRC calculat = %d, primit = %d\n", crc_calculat, crc_primit);
		memset(&r, 0, sizeof(r));
		memcpy(&r, y, sizeof(r));
		memset(&t, 0, sizeof(msg));
		memset(&p, 0, sizeof(packet));
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

	//process S packet

	while(1){
		while ((y = receive_message_timeout(TIME * 1000)) == NULL
			&& retransmitted < 4){
			retransmitted++;
			send_message(&t);
			printf("[%s]: Tot trimit cu retransmit = %d\n", __FILE__, retransmitted);
		}
		if (retransmitted == 4){
			//stop connection
			goto RELEASE;
		} else {
			memset(&p, 0, sizeof(p));
			memcpy(&p, y->payload, sizeof(p));
			show_packet(p);
			crc_calculat = crc16_ccitt(&p, sizeof(packet)-4);
			crc_primit = p.check;
			printf("[%s]: CRC calculat = %d, CRC primit = %d\n", __FILE__, crc_calculat, crc_primit);
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
				printf("Am primit cu erori 2\n");
			} else {
				type = p.type; //save the received type and reuse packet struct
				p.type = Y;
				printf("Am primit fara erori 2\n");
			}
			memcpy(&t.payload, &p, sizeof(p));
			t.len = sizeof(p);
			rc = send_message(&t);
			DIE (rc < 0, "Cannot send ACK for parameter packet");
			retransmitted = 0;
			
			if (crc_calculat != crc_primit){
				printf("Tot gresit\n");
				continue;
			}
			//luare actiune pentru fiecare tip de mesaj
			show_packet(p);	
			switch(type){
				case S:{
					break;
				}
				case F:{
					printf("[%s]: Am primit un mesaj de tip F\n", __FILE__);
					if (f == NULL){
						char filename[MAXL];
						memset(filename, 0, MAXL);
						sprintf(filename, "recv_");
						strcat(filename, p.data);
						f = fopen(filename, "a+");
					}
					break;
				}
				case D:{
					printf("[%s]: Am primit un mesaj de tip D\n", __FILE__);
					if (f != NULL){
						fwrite(p.data, MAXL, 1, f);
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

	//TODO While (!received EOT)

	//TODO parse each message parameters

	//TODO receive(file_heoaders)

	//TODO create_files_on_disk

	//TODO receive(file_data)

	//TODO calculate_crc()

	//TODO Send ACK/NACK

	//TODO Write_in_file

	//TODO Close file
	RELEASE:
		if (f != NULL){
			fflush(f);
			fclose(f);
		}
	return 0;
}
