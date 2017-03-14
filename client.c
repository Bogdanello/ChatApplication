#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "common.h"
 
void get_username(char *username)
{
  while(true)
  {
    printf("Enter a username:");
    fflush(stdout);
    memset(username, 0, 1000);
    fgets(username, MAX_USERNAMELEN+1, stdin);
    trim_newline(username);

    if(strlen(username) > MAX_USERNAMELEN)
    {
      puts("Username must be 20 characters or less.");
    } 
    else 
    {
      break;
    }
  }
}

void set_username(connection_t *connection)
{
  message_t msg;
  msg.type = SET_USERNAME;
  strncpy(msg.username, connection->username, MAX_USERNAMELEN);

  if(send(connection->socket, (void*)&msg, sizeof(msg), 0) < 0)
  {
    perror("Send failed");
    exit(1);
  }
}

void stop_client(connection_t *connection)
{
  close(connection->socket);
  exit(0);
}

void connect_to_server(connection_t *connection, char *address, char *port)
{

  while(true)
  {
    get_username(connection->username);
    if ((connection->socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP)) < 0)
    {
        perror("Could not create socket");
    }

    connection->address.sin_addr.s_addr = inet_addr(address);
    connection->address.sin_family = AF_INET;
    connection->address.sin_port = htons(atoi(port));

    if (connect(connection->socket, (struct sockaddr *)&connection->address , sizeof(connection->address)) < 0)
    {
        perror("Connect failed.");
        exit(1);
    }
    set_username(connection);
    message_t msg;
    ssize_t recv_val = recv(connection->socket, &msg, sizeof(message_t), 0);
    if(recv_val < 0)
    {
        perror("recv failed");
        exit(1);
    }
    else if(recv_val == 0)
    {
      close(connection->socket);
      printf("The username \"%s\" is taken, please try another name.\n", connection->username);
      continue;
    }

    break;
  }


  puts("Connected to server.");
  puts("Type /help for usage.");
}


void read_input(connection_t *connection)
{
  char input[255];
  fgets(input, 255, stdin);
  trim_newline(input);

  if(strcmp(input, "/q") == 0 || strcmp(input, "/quit") == 0)
  {
    stop_client(connection);
  }
  else if(strcmp(input, "/l") == 0 || strcmp(input, "/list") == 0)
  {
    message_t msg;
    msg.type = GET_USERS;

    if(send(connection->socket, &msg, sizeof(message_t), 0) < 0)
    {
        perror("Send failed");
        exit(1);
    }
  }
  else if(strcmp(input, "/h") == 0 || strcmp(input, "/help") == 0)
  {
    puts("/quit or /q: Exit the program.");
    puts("/help or /h: Displays help information.");
    puts("/list or /l: Displays list of users in chatroom.");
    puts("/m <username> <message> Send a private message to given username.");
  }
  else if(strncmp(input, "/m", 2) == 0)
  {
    message_t msg;
    msg.type = PRIVATE_MESSAGE;

    char *toUsername, *chatMsg;

    toUsername = strtok(input+3, " ");

    if(toUsername == NULL)
    {
      puts(KRED "The format for private messages is: /m <username> <message>" RESET);
      return;
    }

    if(strlen(toUsername) == 0)
    {
      puts(KRED "You must enter a username for a private message." RESET);
      return;
    }

    if(strlen(toUsername) > 20)
    {
      puts(KRED "The username must be between 1 and 20 characters." RESET);
      return;
    }

    chatMsg = strtok(NULL, "");

    if(chatMsg == NULL)
    {
      puts(KRED "You must enter a message to send to the specified user." RESET);
      return;
    }

   
    strncpy(msg.username, toUsername, 20);
    strncpy(msg.data, chatMsg, 255);

    if(send(connection->socket, &msg, sizeof(message_t), 0) < 0)
    {
        perror("Send failed");
        exit(1);
    }

  }
  else 
  {
    message_t msg;
    msg.type = PUBLIC_MESSAGE;
    strncpy(msg.username, connection->username, 20);
	 
    if(strlen(input) == 0) {
        return;
    }
    strncpy(msg.data, input, 255);

    if(send(connection->socket, &msg, sizeof(message_t), 0) < 0)
    {
        perror("Send failed");
        exit(1);
    }
  }



}

void handle_server_message(connection_t *connection)
{
  message_t msg;

  ssize_t recv_val = recv(connection->socket, &msg, sizeof(message_t), 0);
  if(recv_val < 0)
  {
      perror("recv failed");
      exit(1);

  }
  else if(recv_val == 0)
  {
    close(connection->socket);
    puts("Server disconnected.");
    exit(0);
  }

  switch(msg.type)
  {

    case CONNECT:
      printf(KYEL "%s has connected." RESET "\n", msg.username);
    break;

    case DISCONNECT:
      printf(KYEL "%s has disconnected." RESET "\n" , msg.username);
    break;

    case GET_USERS:
      printf("%s", msg.data);
    break;

    case PUBLIC_MESSAGE:
      printf(KGRN "%s" RESET ": %s\n", msg.username, msg.data);
    break;

    case PRIVATE_MESSAGE:
      printf(KWHT "From %s:" KCYN " %s\n" RESET, msg.username, msg.data);
    break;

    case TOO_FULL:
      fprintf(stderr, KRED "Server reached maximum number of clients." RESET "\n");
      exit(0);
    break;

    default:
      fprintf(stderr, KRED "Unknown message type received." RESET "\n");
    break;
  }
}

int main(int argc, char *argv[])
{
  connection_t connection;
  fd_set file_descriptors;

  if (argc != 3) {
    fprintf(stderr,"Usage: %s <IP> <port>\n", argv[0]);
    exit(1);
  }

  connect_to_server(&connection, argv[1], argv[2]);

  //keep communicating with server
  while(true)
  {
    FD_ZERO(&file_descriptors);
    FD_SET(STDIN_FILENO, &file_descriptors);
    FD_SET(connection.socket, &file_descriptors);
    fflush(stdin);

    if(select(connection.socket+1, &file_descriptors, NULL, NULL, NULL) < 0)
    {
      perror("Select failed.");
      exit(1);
    }

    if(FD_ISSET(STDIN_FILENO, &file_descriptors))
    {
      read_input(&connection);
    }

    if(FD_ISSET(connection.socket, &file_descriptors))
    {
      handle_server_message(&connection);
    }
  }

  close(connection.socket);
  return 0;
}
 
