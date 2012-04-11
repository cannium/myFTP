#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "users.h"
#include "command.h"

#define LISTEN_PORT		21
#define LISTEN_QUEUE	128

#define BUFFER_SIZE		1024

// FIXME fix users
// TODO  add login procedure

int listenSocketFileDescriptor;
int maxFileDescriptor;
struct sockaddr_in serverAddress, clientAddress;
extern user* connectedUser;
char buffer[BUFFER_SIZE];

void initialize();
void startServer();
void mainLoop();


int main(int argc, char* argv[])
{
	initialize();
	startServer();
	mainLoop();
}

void initialize()
{
	/* TODO:
	   setup SIGINT handler;
	   open configuration file;
	*/

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(LISTEN_PORT);
}

void startServer()
{
	listenSocketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSocketFileDescriptor < 0)
	{
		fprintf(stderr, "error creating socket\n");
		exit(-1);
	}

	if( bind(listenSocketFileDescriptor, (struct sockaddr *) &serverAddress, \
				sizeof(serverAddress)) < 0 )
	{
		fprintf(stderr, "can't bind to port %d\n", LISTEN_PORT);
		exit(-1);
	}

	if( listen(listenSocketFileDescriptor, LISTEN_QUEUE) < 0)
	{
		fprintf(stderr, "error converting to passive socket\n");
		exit(-1);
	}

}

void mainLoop()
{
	fd_set readSet,tempSet;
	
	maxFileDescriptor = listenSocketFileDescriptor;
	FD_ZERO(&readSet);
	FD_SET(listenSocketFileDescriptor, &readSet);

	while(1)
	{
		tempSet = readSet;
		int readyNumber = select(maxFileDescriptor + 1, &tempSet, NULL, \
									NULL, NULL);
		if(readyNumber < 0)
		{
			fprintf(stderr, "select error\n");
			exit(-1);
		}

		if( FD_ISSET(listenSocketFileDescriptor, &tempSet))
		{
			socklen_t addressLength = sizeof(clientAddress);
			int connectingFileDescriptor;
			connectingFileDescriptor = accept(listenSocketFileDescriptor, \
									(struct sockaddr *) &clientAddress,	\
									&addressLength);
			if(connectingFileDescriptor < 0)
			{
				fprintf(stderr, "accept error\n");
				exit(-1);
			}

			FD_SET(connectingFileDescriptor, &readSet);

			if(connectingFileDescriptor > maxFileDescriptor)
				maxFileDescriptor = connectingFileDescriptor;

			addNewUser(connectingFileDescriptor);

			readyNumber--;
			if(readyNumber <= 0)
				continue;
		}

		user* current;
		for(current = connectedUser; current != NULL; current = current -> next)
		{
			int fd = current -> socketFileDescriptor;
			if( FD_ISSET(fd, &tempSet))
			{
				ssize_t n;
				n = read(fd, buffer, BUFFER_SIZE);
				if(n == 0)
				{
					close(fd);
					removeUser(fd);
					FD_CLR(fd, &readSet);

					if(fd == maxFileDescriptor)
						maxFileDescriptor--;
				}
				else
					handleCommand(buffer, n);

				readyNumber--;
				if(readyNumber <= 0)
					break;
			}
		}
	}
}
