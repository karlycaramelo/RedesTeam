#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef enum {GET, POST, DEFAULT} request_type;
void start_server(int);
void error(const char*);
void respond(int);
void help();
char *read_file_to_buffer(char *);
void send_response(int, char*, int);
