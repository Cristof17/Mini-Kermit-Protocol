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
	int seq = 0;
	int seq_asteptat = 0;
	int seq_trimis = 0;
	uint16_t crc = 0;
	int retransmitted =0;
	msg *y;
	char buffer[MAXL];
	int numBytes;
	FILE *f = NULL;
	int type = 0;
	//TODO Save current packet and current index

    init(HOST, PORT);

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
	p.soh = soh;
	p.len = len;
	p.seq = seq_trimis;
	p.type = S;
	p.mark = MARK;
	memcpy(&p.data, &s, sizeof(s_packet));
	//len - 3 means the length of the message minus MARK + CHECK
	crc= crc16_ccitt(&p, sizeof(packet)-4);
	p.check = crc;
	//show_packet(p);
	//the size of the status fields is the len + the first two fields
	memcpy(&t.payload, &p, sizeof(p));
	//SOH + LEN + Len
	t.len = sizeof(p);
	//Send s
	rc = send_message(&t);
	seq_trimis += 2;
	seq_asteptat = seq_trimis - 1;
	DIE(rc < 0, "Cannot send message S");

	WAIT_ACK_S:
	//while has to receive packets
	while ((y = receive_message_timeout(TIME * 1000)) == NULL){
		if (retransmitted == 3){
			//TODO Stop transmission
			//stop connection
			goto RELEASE;
		}else{
			rc = send_message(&t);
			DIE(rc < 0, "Cannot resend send message S");
			retransmitted++;
			print_status(retransmitted);
		}
	}
	retransmitted = 0;
	memset(&p, 0, sizeof(p));
	memcpy(&p, y->payload, sizeof(p));
	printf("[%s]: seq_trimis = %d seq_asteptat = %d\n", __FILE__, seq_trimis, seq_asteptat);
	if (p.type == N){
		memset(&p, 0, sizeof(p));
		memcpy(&p, &t.payload, sizeof(p));
		p.seq = seq_trimis;
		crc = crc16_ccitt(&p, sizeof(packet) - 4);
		p.check = crc;
		memset(&t, 0, sizeof(t));
		memcpy(&t.payload, &p, sizeof(p));
		t.len = sizeof(p);
		rc = send_message(&t);
		seq_trimis += 2;
		seq_asteptat = seq_trimis - 1;
		DIE(rc < 0, "Cannot send S packet again");
		retransmitted = 0;
		goto WAIT_ACK_S;
	} else {
		printf("[%s]; Am primit ACK pentru S cu seq %d\n", __FILE__, p.seq);
	}
	
	for (i = 1; i < argc; ++i){
		//send F message
		memset(&p, 0, sizeof(p));
		memset(&t, 0, sizeof(msg));
		len = sizeof(p) - 2;
		p.soh = soh;
		p.len = len;
		p.seq = seq_trimis;
		p.type = F;
		sprintf(p.data, "%s", argv[i]);
		f = fopen(argv[i], "r+");
		printf("[%s]: Sending type %d\n", __FILE__, p.type);
		//show_packet(p);
		//len - 4 means the length of the message minus MARK + CHECK + PADDING
		crc= crc16_ccitt(&p, sizeof(p) - 4);
		p.check = crc;
		//the size of the status fields is the len + the first two fields
		memcpy(&t.payload, &p, sizeof(p));
		//SOH + LEN + Len
		t.len = sizeof(p);
		//Send s
		rc = send_message(&t);
		seq_trimis +=2;
		seq_asteptat = seq_trimis - 1;
		DIE(rc < 0, "Cannot send message S");

		WAIT_ACK_F:
		//while has to receive packets
		while ((y = receive_message_timeout(TIME * 1000)) == NULL){
			if (retransmitted == 3){
				//TODO Stop transmission
				//stop connection
				goto RELEASE;
			}else{
				rc = send_message(&t);
				DIE(rc < 0, "Cannot resend send message S");
				retransmitted++;
				print_status(retransmitted);
			}
		}
		retransmitted = 0;
		memset(&p, 0, sizeof(p));
		memcpy(&p, y->payload, sizeof(p));
		printf("[%s]: seq_trimis = %d seq_asteptat = %d\n", __FILE__, seq_trimis, seq_asteptat);
		if (p.type == Y && seq_asteptat != p.seq){
			rc = send_message(&t);
			DIE(rc < 0, "Cannot send ACK");
			goto WAIT_ACK_F;
		}
		else if (p.type == N ){
			printf("[%s]: Am primit NACK pentru F cu seq %d\n", __FILE__, p.seq);
			memset(&p, 0, sizeof(p));
			memcpy(&p, &t.payload, sizeof(p));
			p.seq = seq_trimis;
			crc = crc16_ccitt(&p, sizeof(p) -4);
			p.check = crc;
			memset(&t, 0, sizeof(t));
			memcpy(&t.payload, &p, sizeof(p));
			t.len = sizeof(p);
			rc = send_message(&t);
			seq_trimis += 2;
			seq_asteptat = seq_trimis - 1;
			DIE(rc < 0, "Cannot send S packet again");
			retransmitted = 0;
			goto WAIT_ACK_F;
		} else {
			printf("[%s]; Am primit ACK pentru F cu seq %d\n", __FILE__, p.seq);
		}
		//show_packet(p);

		READ_DATA:
		//send D message
		memset(&p, 0, sizeof(p));
		memset(&t, 0, sizeof(msg));
		len = sizeof(p) - 2;
		p.soh = soh;
		p.seq = seq_trimis;
		p.type = D;
		numBytes = fread(p.data, 1, MAXL, f);
		printf("[%s] Am citit %d\n", __FILE__, numBytes);
		p.len = numBytes;
		printf("[%s]: Sending type %d %d\n", __FILE__, p.type, p.seq);
		//len - 4 means the length of the message minus MARK + CHECK + PADDING
		crc= crc16_ccitt(&p, sizeof(p) - 4);
		p.check = crc;
		//show_packet(p);
		//the size of the status fields is the len + the first two fields
		memcpy(&t.payload, &p, sizeof(p));
		//SOH + LEN + Len
		t.len = sizeof(p);
		//Send s
		rc = send_message(&t);
		seq_trimis += 2;
		seq_asteptat = seq_trimis - 1;
		DIE(rc < 0, "Cannot send message S");

		WAIT_ACK_D:
		//while has to receive packets
		while ((y = receive_message_timeout(TIME * 1000)) == NULL){
			if (retransmitted == 3){
				//TODO Stop transmission
				//stop connection
				goto RELEASE;
			}else{
				rc = send_message(&t);
				DIE(rc < 0, "Cannot resend send message S");
				retransmitted++;
				print_status(retransmitted);
			}
		}
		retransmitted = 0;
		memset(&p, 0, sizeof(p));
		memcpy(&p, y->payload, sizeof(p));
		printf("[%s]: seq_trimis = %d seq_asteptat = %d\n", __FILE__, seq_trimis, seq_asteptat);
		if ( p.type == Y && seq_asteptat != p.seq){
			rc = send_message(&t);
			DIE(rc < 0, "Cannot resend prev ACK message");
			goto WAIT_ACK_D;
		}
		else if (p.type == N){
			printf("[%s]: Am primit NACK pentru D cu seq %d\n", __FILE__, p.seq);
			memset(&p, 0, sizeof(p));
			memcpy(&p, &t.payload, sizeof(p));
			p.seq = seq_trimis;
			crc = crc16_ccitt(&p, sizeof(p) -4);
			p.check = crc;
		//	memset(&t, 0, sizeof(t));
			memcpy(&t.payload, &p, sizeof(p));
			t.len = sizeof(p);
			rc = send_message(&t);
			seq_trimis += 2;
			seq_asteptat = seq_trimis - 1;
			DIE(rc < 0, "Cannot send S packet again");
			retransmitted = 0;
			goto WAIT_ACK_D;
		} else {
			printf("[%s]: Am primit ACK pentru D cu seq %d\n", __FILE__, p.seq);
		}
		
		if (numBytes != 0){
			goto READ_DATA;
		}
		//show_packet(p);

		//send EOF message
		memset(&p, 0, sizeof(p));
		memset(&t, 0, sizeof(msg));
		len = sizeof(p) - 2;
		p.soh = soh;
		p.len = len;
		p.seq = seq_trimis;
		p.type = Z;
		numBytes = fread(p.data, MAXL, 1, f);
		if (numBytes == 0)
			p.type = Z;
		printf("[%s]: Sending type %d\n", __FILE__, p.type);
		//show_packet(p);
		//len - 4 means the length of the message minus MARK + CHECK + PADDINGm primit ACK pentru D cu seq 5
		crc= crc16_ccitt(&p, sizeof(p) - 4);
		p.check = crc;
		//the size of the status fields is the len + the first two fields
		memcpy(&t.payload, &p, sizeof(p));
		//SOH + LEN + Len
		t.len = sizeof(p);
		//Send s
		rc = send_message(&t);
		seq_trimis += 2;
		seq_asteptat = seq_trimis - 1;
		DIE(rc < 0, "Cannot send message S");

		WAIT_ACK_Z:
		//while has to receive packets
		while ((y = receive_message_timeout(TIME * 1000)) == NULL){
			if (retransmitted == 3){
				//TODO Stop transmission
				//stop connection
				goto RELEASE;
			}else{
				rc = send_message(&t);
				DIE(rc < 0, "Cannot resend send message S");
				retransmitted++;
				print_status(retransmitted);
			}
		}
		retransmitted = 0;
		memset(&p, 0, sizeof(p));
		memcpy(&p, y->payload, sizeof(p));
		printf("[%s]: seq_trimis = %d seq_asteptat = %d\n", __FILE__, seq_trimis, seq_asteptat);
		if ( p.type == Y && seq_asteptat != p.seq){
			rc = send_message(&t);
			DIE(rc < 0, "Cannot resend prev ACK message");
			goto WAIT_ACK_Z;
		}
		else if (p.type == N){
			printf("[%s]: Am primit NACK pentru Z cu seq %d\n", __FILE__, p.seq);
			memset(&p, 0, sizeof(p));
			memcpy(&p, &t.payload, sizeof(p));
			p.seq = seq_trimis;
			crc = crc16_ccitt(&p, sizeof(p) -4);
			p.check = crc;
			memset(&t, 0, sizeof(t));
			memcpy(&t.payload, &p, sizeof(p));
			t.len = sizeof(p);
			rc = send_message(&t);
			seq_trimis += 2;
			seq_asteptat = seq_trimis - 1;
			DIE(rc < 0, "Cannot send S packet again");
			retransmitted = 0;
			goto WAIT_ACK_Z;
		}else {
			printf("[%s]: Am primit ACK pentru Z cu seq %d\n", __FILE__, p.seq);
		}
		//show_packet(p);

		if (i == argc-1){
			//send EOF message
			memset(&p, 0, sizeof(p));
			memset(&t, 0, sizeof(msg));
			len = sizeof(p) - 2;
			p.soh = soh;
			p.len = len;
			p.seq = seq_trimis;
			p.type = B;
			printf("[%s]: Sending type %d\n", __FILE__, p.type);
			//show_packet(p);
			//len - 4 means the length of the message minus MARK + CHECK + PADDING
			crc= crc16_ccitt(&p, sizeof(p) - 4);
			p.check = crc;
			//the size of the status fields is the len + the first two fields
			memcpy(&t.payload, &p, sizeof(p));
			//SOH + LEN + Len
			t.len = sizeof(p);
			//Send s
			rc = send_message(&t);
			seq_trimis += 2;
			seq_asteptat = seq_trimis - 1;
			DIE(rc < 0, "Cannot send message S");

			WAIT_ACK_B:
			//while has to receive packets
			while ((y = receive_message_timeout(TIME * 1000)) == NULL){
				if (retransmitted == 3){
					//TODO Stop transmission
					//stop connection
					goto RELEASE;
				}else{
					rc = send_message(&t);
					DIE(rc < 0, "Cannot resend send message S");
					retransmitted++;
					print_status(retransmitted);
				}
			}
			retransmitted = 0;
			memset(&p, 0, sizeof(p));
			memcpy(&p, y->payload, sizeof(p));
			printf("[%s]: seq_trimis = %d seq_asteptat = %d\n", __FILE__, seq_trimis, seq_asteptat);
			if ( p.type == Y && seq_asteptat != p.seq){
				rc = send_message(&t);
				DIE(rc < 0, "Cannot resend prev ACK message");
				goto WAIT_ACK_B;
			}
			if (p.type == N){
				printf("[%s]: Am primit NACK pentru B cu seq %d\n", __FILE__, seq);
				memset(&p, 0, sizeof(p));
				memcpy(&p, &t.payload, sizeof(p));
				p.seq = seq;
				crc = crc16_ccitt(&p, sizeof(p) -4);
				p.check = crc;
				memset(&t, 0, sizeof(t));
				memcpy(&t.payload, &p, sizeof(p));
				t.len = sizeof(p);
				rc = send_message(&t);
				seq_trimis += 2;
				seq_asteptat = seq_trimis - 1;
				DIE(rc < 0, "Cannot send S packet again");
				retransmitted = 0;
				goto WAIT_ACK_B;
			} else { 
				printf("[%s]: Am primit ACK pentru B cu seq %d\n", __FILE__, p.seq);
			}
		}
	}


	RELEASE:
		printf("[%s]: Aici\n", __FILE__);
		if (f != NULL){
			fflush(f);
			fclose(f);
		}
		printf("[%s]: Aici\n", __FILE__);
    return 0;
}
