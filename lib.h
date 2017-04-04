#ifndef LIB
#define LIB

typedef struct {
	uint8_t soh;
	uint8_t len;
	uint8_t seq;
	uint8_t type;
	char *data;
	uint8_t check;
	uint8_t mark;
}

typedef struct {
	uint8_t maxl;
	uint8_t time;
	uint8_t npad;
	uint8_t padc;
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
    char payload[1400];
} msg;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif

