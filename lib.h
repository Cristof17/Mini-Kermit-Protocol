#ifndef LIB
#define LIB
#include <stdint.h>

#define S 10
#define F 20
#define D 30
#define Z 40
#define B 50
#define Y 60
#define N 70

#define PAYLOAD_LEN 1400
#define MAXL 		250

#define OK 		1
#define NOT_OK 	0

#define show_packet(p) printf("[%s] %x %x %x %x %hu %s\n",__FILE__, p.soh, p.len, p.seq, p.type, p.check, p.data);

#define print_crc(p) printf("[%s] %u\n",__FILE__, p.check);

#define print_data(m) printf("[%s] %s\n", __FILE__, m.payload)

#define print_message(m) printf("[%s] %d %s\n", __FILE__, m.len, m.payload);

#define print_stats(s) printf("[%s] %d %d %d %c %c\n", __FILE__, s.maxl, s.time,s.npad, s.padc, s.eol);

#define print_status(i) printf("[%s] Aștept cu i = %d\n", __FILE__, i);

#define SOH		0x01
#define MAXL	250
#define TIME 	5 //seconds
#define NPAD 	0x00
#define PADC	0x00
#define EOL 	0x0D
#define MARK 	0x0D

typedef struct {
	uint8_t soh;
	uint8_t len;
	uint8_t seq;
	uint8_t type;
	char 	data[MAXL];
	uint16_t check;
	uint8_t mark;
} packet;

typedef struct {
	uint8_t maxl;
	uint8_t time;
	uint8_t npad;
	uint8_t padc; //if npad is 0x00 this is ignored
	uint8_t eol;
	uint8_t qctl;
	uint8_t qbin;
	uint8_t chkt;
	uint8_t rept;
	uint8_t capa;
	uint8_t r;
} s_packet;

typedef struct {
    int len;
    char payload[PAYLOAD_LEN];
} msg;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif

