#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "global.h"
#include "users.h"
#include "command.h"

#define LISTEN_PORT		21
#define LISTEN_QUEUE	128

#define FD_SET_SIZE		32

int listenSocketFileDescriptor;
int maxFileDescriptor;
fd_set readSet;
struct sockaddr_in serverAddress, clientAddress;
char buffer[BUFFER_SIZE];

extern userList connectedUser;
extern userList unconnectedUser;

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
	   setup SIGINT SIGCHLD handler;
	*/
	FILE* configurationFile;
	configurationFile = fopen("ftp.conf", "r");
	if(!configurationFile)
	{
		fprintf(stderr, "error open configuration file\n");
		exit(-1);
	}

	while( !feof(configurationFile))
	{
		if( fgets(buffer, BUFFER_SIZE, configurationFile) == NULL)
			continue;

		if(buffer[0] == '#' || buffer[0] == '\n')
			continue;

		user* newUser = addNewUser(&unconnectedUser);
		if(!newUser)
		{
			fprintf(stderr, "error adding new user\n");
			exit(-1);
		}

		char upload[4], download[4], speedLimit[32]; 		
		sscanf(buffer, "%63s %63s %1023s %s %s %s", newUser -> username,
				newUser -> password, newUser -> homeDirectory, upload, download,
				speedLimit);
		if(upload[0] == 'Y')
			newUser -> upload = ENABLE;
		else
			newUser -> upload = DISABLE;

		if(download[0] == 'Y')
			newUser -> download = ENABLE;
		else
			newUser -> download = DISABLE;

		newUser -> speedLimit = atoi(speedLimit);
	}
	if(DEBUG)
		printUserList(&unconnectedUser);
}

void startServer()
{
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(LISTEN_PORT);

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

void removeSocket(int fileDescriptor)
{
	close(fileDescriptor);
	FD_CLR(fileDescriptor, &readSet);

	if(fileDescriptor == maxFileDescriptor)
		maxFileDescriptor--;
}

void mainLoop()
{
	fd_set tempSet;
	int i;
	ssize_t n;
	
	maxFileDescriptor = listenSocketFileDescriptor;
	FD_ZERO(&readSet);
	FD_SET(listenSocketFileDescriptor, &readSet);

	int unknownUserFileDescriptor[FD_SET_SIZE] = {0};
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

			reply(connectingFileDescriptor, SERVICE_READY, "welcome!");

			for(i = 0; i < FD_SET_SIZE; i++)
			{
				if(unknownUserFileDescriptor[i] == 0)
					break;
			}
			unknownUserFileDescriptor[i] = connectingFileDescriptor;

			FD_SET(connectingFileDescriptor, &readSet);

			if(connectingFileDescriptor > maxFileDescriptor)
				maxFileDescriptor = connectingFileDescriptor;

			readyNumber--;
			if(readyNumber <= 0)
				continue;
		}

		user* current;
		for(current = connectedUser.first; current; 
							current = current -> next)
		{
			int fd = current -> socketFileDescriptor;
			if( FD_ISSET(fd, &tempSet))
			{
				n = read(fd, buffer, BUFFER_SIZE);
				if(n == 0)
				{
					moveUser(&connectedUser, &unconnectedUser, current);
					removeSocket(fd);					
				}
				else
				{
					buffer[n] = '\0';
					handleCommand(current, buffer, n);
				}

				readyNumber--;
				if(readyNumber <= 0)
					break;
			}
		}

				
		for(i = 0; i < FD_SET_SIZE; i++)
		{
			int fd = unknownUserFileDescriptor[i];
			if(!fd)
				continue;
			if( !FD_ISSET(fd, &tempSet))
				continue;
			
			n = read(fd, buffer, BUFFER_SIZE);
			if(n == 0)
			{
				unknownUserFileDescriptor[i] = 0;
				removeSocket(fd);				
			}
			else
			{
				char temp[REQUEST_BUFF], name[NAME_LENGTH];
				sscanf(buffer, "%4s %63s", temp, name);
				if( strcmp(temp, "USER") == 0)
				{
					reply(fd, NEED_PASSWORD, "specify your password");
					user* connecting = findUserByName(name, &unconnectedUser);
					if(connecting)
					{
						moveUser(&unconnectedUser, &connectedUser, connecting);
						connecting -> socketFileDescriptor = fd;
						unknownUserFileDescriptor[i] = 0;				
					}
				}
				else
					removeSocket(fd);
			}
							
			readyNumber--;
			if(readyNumber <= 0)
				break;
		}

		if(DEBUG)
		{
			printf("unconnectedUser:\n");
			printUserList(&unconnectedUser);
			printf("connectedUser:\n");
			printUserList(&connectedUser);
			printf("\n");
		}

	}
}
