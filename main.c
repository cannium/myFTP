#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "global.h"
#include "users.h"
#include "command.h"

#define FD_SET_SIZE		32

int listenSocketFileDescriptor;
extern int maxFileDescriptor;
extern fd_set readSet;
struct sockaddr_in serverAddress, clientAddress;
char buffer[BUFFER_SIZE];

extern userList connectedUser;
extern userList unconnectedUser;

void initialize();
void mainLoop();
void singalInterruptionHandler(int signo);


int main(int argc, char* argv[])
{
	initialize();
	listenSocketFileDescriptor = startServer(&serverAddress);
	printf("server started\n");
	mainLoop();
}

void initialize()
{
	/* TODO:
	   setup SIGCHLD handler;
	*/

	signal(SIGINT, singalInterruptionHandler);

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

		char writeAccess[4], readAccess[4], speedLimit[32]; 		
		sscanf(buffer, "%63s %63s %1023s %s %s %s", newUser -> username,
				newUser -> password, newUser -> homeDirectory, writeAccess,
			   	readAccess, speedLimit);
		if(writeAccess[0] == 'Y')
			newUser -> writeAccess = ENABLE;
		else
			newUser -> writeAccess = DISABLE;

		if(readAccess[0] == 'Y')
			newUser -> readAccess = ENABLE;
		else
			newUser -> readAccess = DISABLE;

		newUser -> speedLimit = atoi(speedLimit);
	}
	if(DEBUG)
		printUserList(&unconnectedUser);

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(LISTEN_PORT);
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
			int fd = current -> controlSocket;
			if(fd == 0)
				continue;

			if( FD_ISSET(fd, &tempSet))
			{
				n = read(fd, buffer, BUFFER_SIZE);
				if(n == 0)
				{
					moveUser(&connectedUser, &unconnectedUser, current);
					current -> controlSocket = 0;
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
						connecting -> controlSocket = fd;
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

void singalInterruptionHandler(int signo)
{
	user* current;
	for(current = connectedUser.first; current; current = current -> next)
	{
		reply(current -> controlSocket, SERVIVE_CLOSE, "server is closed");
		close(current -> controlSocket);
	}
	close(listenSocketFileDescriptor);
	printf("\nserver is closed\n");
	exit(0);
}
