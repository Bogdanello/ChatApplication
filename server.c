
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>

#include "common.h"

#define MAX_CLIENTS 4

void init(connection_t *server_connection, int port)
{
  if((server_connection->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Failure! Creating socket didn't succeed!");
    exit(1);
  }

  server_connection->address.sin_family = AF_INET;
  server_connection->address.sin_addr.s_addr = INADDR_ANY;
  server_connection->address.sin_port = htons(port);

  if(bind(server_connection->socket, (struct sockaddr *)&server_connection->address, sizeof(server_connection->address)) < 0)
  {
    perror("Binding failed");
    exit(1);
  }
  // set SO_REUSEADDR on a socket to true (1):
  // Allows other sockets to bind() to this port, unless there is an active listening socket bound to the port already
  const int optVal = 1;
  const socklen_t optLen = sizeof(optVal);

  if(setsockopt(server_connection->socket, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, optLen) < 0)
  {
    perror("Setting socket failed");
    exit(1);
  }


  if(listen(server_connection->socket, 3) < 0) {
    perror("Listening failed");
    exit(1);
  }

   printf("Server Started \nWaiting for incoming clients to connect :) ...\n");
}

void stop_server(connection_t connection[])
{

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		close(connection[i].socket);
	}
	exit(0);
}

void send_private_message(connection_t clients[], int sender,
	char *username, char *message)
{
	message_t msg;
	msg.type = PRIVATE_MESSAGE;
	strncpy(msg.username, clients[sender].username, MAX_USERNAMELEN);
	strncpy(msg.data, message, 256);

	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (i != sender && clients[i].socket != 0
			&& strcmp(clients[i].username, username) == 0)
		{
			if (send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
			{
				perror("Send failed");
				exit(1);
			}
			return;
		}
	}

	msg.type = USERNAME_ERROR;
	sprintf(msg.data, "Username \"%s\" does not exist or is not logged in.", username);

	if (send(clients[sender].socket, &msg, sizeof(msg), 0) < 0)
	{
		perror("Send failed");
		exit(1);
	}

}

void send_message_toAll(connection_t clients[], int sender, char *message)
{
  message_t msg;
  msg.type = PUBLIC_MESSAGE;
  strncpy(msg.username, clients[sender].username, MAX_USERNAMELEN);
  strncpy(msg.data, message, 256);
 
  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(i != sender && clients[i].socket != 0)
    {
      if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
      {
          perror("Send failed");
          exit(1);
      }
    }
  }
}



void client_connected_message(connection_t *clients, int sender)
{
  message_t msg;
  msg.type = CONNECT;
  strncpy(msg.username, clients[sender].username, MAX_USERNAMELEN);
   
  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket != 0)
    {
      if(i == sender)
      {
        msg.type = SUCCESS;
        if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
        {
            perror("Send failed");
            exit(1);
        }
      }else
      {
        if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
        {
            perror("Send failed");
            exit(1);
        }
      }
    }
  }
}

void client_disconnected_message(connection_t *clients, char *username)
{
  message_t msg;
  msg.type = DISCONNECT;
  strncpy(msg.username, username, MAX_USERNAMELEN);
  
  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket != 0)
    {
      if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
      {
          perror("Send failed");
          exit(1);
      }
    }
  }
}

void show_online_users(connection_t *clients, int receiver) {
  message_t msg;
  msg.type = GET_USERS;
  char *list = msg.data;

  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket != 0)
    {
      list = stpcpy(list, clients[i].username);
      list = stpcpy(list, "\n");
    }
  }

  if(send(clients[receiver].socket, &msg, sizeof(msg), 0) < 0)
  {
      perror("Send failed");
      exit(1);
  }

}

void maximum_number_reached(int socket)
{
  message_t too_full_message;
  too_full_message.type = TOO_FULL;

  if(send(socket, &too_full_message, sizeof(too_full_message), 0) < 0)
  {
      perror("Send failed");
      exit(1);
  }

  close(socket);
}

 



void handle_client_message(connection_t clients[], int sender)
{
  int read_size;
  message_t msg;

  if((read_size = recv(clients[sender].socket, &msg, sizeof(message_t), 0)) == 0)
  {
    printf("User offline: %s.\n", clients[sender].username);
    close(clients[sender].socket);
    clients[sender].socket = 0;
    client_disconnected_message(clients, clients[sender].username);

  } 
  else
	{

    switch(msg.type)
    {
      case GET_USERS:
        show_online_users(clients, sender);
      break;

      case SET_USERNAME: 
      
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
          if(clients[i].socket != 0 && strcmp(clients[i].username, msg.username) == 0)
          {
            close(clients[sender].socket);
            clients[sender].socket = 0;
            return;
          }
        }

        strcpy(clients[sender].username, msg.username);
        printf("User connected: %s\n", clients[sender].username);
        client_connected_message(clients, sender);
      break;

      case PUBLIC_MESSAGE:
        send_message_toAll(clients, sender, msg.data);
      break;

      case PRIVATE_MESSAGE:
        send_private_message(clients, sender, msg.username, msg.data);
      break;

      default:
        fprintf(stderr, "Unknown message type received.\n");
      break;
    }
  }
}

int construct_fd_set(fd_set *set, connection_t *server_info,
                      connection_t clients[])
{
  //Initializes the file descriptor set fdset to have zero bits for all file descriptors.
  FD_ZERO(set);
  //Sets the bit for the file descriptor fd in the file descriptor set fdset.
  FD_SET(STDIN_FILENO, set);
  FD_SET(server_info->socket, set);

  int max_fd = server_info->socket;
  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket > 0)
    {
      FD_SET(clients[i].socket, set);
      if(clients[i].socket > max_fd)
      {
        max_fd = clients[i].socket;
      }
    }
  }
  return max_fd;
}

void handle_new_connection(connection_t *server_info, connection_t clients[])
{
  int new_socket;
  int address_len;
  new_socket = accept(server_info->socket, (struct sockaddr*)&server_info->address, (socklen_t*)&address_len);

  if (new_socket < 0)
  {
    perror("Accept Failed");
    exit(1);
  }

  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i].socket == 0) {
      clients[i].socket = new_socket;
      break;

    } else if (i == MAX_CLIENTS -1) // if we can accept no more clients
    {
      maximum_number_reached(new_socket);
    }
  }
}

void read_input(connection_t clients[])
{
  char input[255];
  fgets(input, sizeof(input), stdin);
  trim_newline(input);

  if(input[0] == 'q') {
    stop_server(clients);
  }
}

int main(int argc, char *argv[])
{
  puts("Starting server.");

  fd_set file_descriptors;

  connection_t server_info;
  connection_t clients[MAX_CLIENTS];
   
  for(int i = 0; i < MAX_CLIENTS; i++)
  {
    clients[i].socket = 0;
  }

  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  int portNo = atoi(argv[1]);
  init(&server_info,portNo);

  while(1)
  {
    int max_fd = construct_fd_set(&file_descriptors, &server_info, clients);
	/*
	* When select returns, it has updated the sets to show which file descriptors have become ready for read/write/exception. 
	* All other flags have been cleared.
	*/
    if(select(max_fd+1, &file_descriptors, NULL, NULL, NULL) < 0)
    {
      perror("Select Failed");
      stop_server(clients);
    }

    if(FD_ISSET(STDIN_FILENO, &file_descriptors))
    {
      read_input(clients);
    }

    if(FD_ISSET(server_info.socket, &file_descriptors))
    {
      handle_new_connection(&server_info, clients);
    }

    for(i = 0; i < MAX_CLIENTS; i++)
    {
      if(clients[i].socket > 0 && FD_ISSET(clients[i].socket, &file_descriptors))
      {
        handle_client_message(clients, i);
      }
    }
  }

  return 0;
}