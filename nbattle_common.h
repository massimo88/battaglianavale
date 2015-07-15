#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

int read_message(int fd, void* buff,size_t len);
int send_string(int fd,char *s);
int send_message(int fd, void* buff,size_t len);
int send_tokens(int fd, const char *tokens[], int num_tokens);

#define MAX_BUFF_LEN 256
#define MAX_ARGS 3
