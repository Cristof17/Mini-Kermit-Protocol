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
	show_packet(p);
	//check params CRC
	crc = crc16_ccitt(&p, sizeof(p) - 3); 
	print_crc(p);
	//TODO Parse parameters

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
