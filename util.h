#include <unistd.h>

#define BUFF_SIZE   (1024)
#define BACKLOG       (12)

//enum {false, true};
//typedef unsigned bool;

void error_msg(const char* msg, bool halt_flag);
int create_server_socket(bool non_blocking);
void announce_client(struct in_addr* addr);
void generate_echo_response(char request[], char response[]);

