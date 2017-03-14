
#ifndef __COMMON__H
#define __COMMON__H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>


//color codes
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

#define MAX_USERNAMELEN 21

typedef enum
{
  CONNECT,
  DISCONNECT,
  GET_USERS,
  SET_USERNAME,
  PUBLIC_MESSAGE,
  PRIVATE_MESSAGE,
  TOO_FULL,
  USERNAME_ERROR,
  SUCCESS,
  ERROR

} message_type;

typedef struct
{
  message_type type;
  char username[21];
  char data[256];

} message_t;

typedef struct connection
{
  int socket;
  struct sockaddr_in address;
  char username[20];
} connection_t;

void trim_newline(char *text);
void clear_stdin_buffer();

#endif