#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include<fcntl.h>

#include "global.h"
#include "command.h"
#include "users.h"

extern userList connectedUser, unconnectedUser;

int maxFileDescriptor;
fd_set readSet;

void handleCommand(user* currentUser, const char* buffer, ssize_t size)
{
	printf("handling user %s : %s\n", currentUser -> username, buffer);

	char request[REQUEST_BUFF] = {0}, parameter[BUFFER_SIZE] = {0};
	sscanf(buffer, "%4s %1023s", request, parameter);

	if(DEBUG)
		printf("%s|%s|\n", request, parameter);

	if( strcmp(request, "PASS") == 0)		// password
	{
		if( strcmp(parameter, currentUser -> password) == 0)
		{
			currentUser -> loginTime = time(NULL);
			strcpy(currentUser -> currentPath,	\
					currentUser -> homeDirectory);
			reply(currentUser -> controlSocket, LOGIN_SUCCESS,	\
				   	"login successful");
			printf("user %s logged in\n", currentUser -> username);

			struct sockaddr_in peerAddress;
			socklen_t addressLength;
			addressLength = sizeof(peerAddress);
			getpeername(currentUser -> controlSocket,	\
					(struct sockaddr*)&peerAddress,		\
					&addressLength);

			printf("IP address: %s:%d\n\n",	\
					inet_ntoa( peerAddress.sin_addr),	\
					(int) ntohs(peerAddress.sin_port));
		}
		else
		{
			reply(currentUser -> controlSocket, LOGIN_FAILED,	\
					"login failed");
		}
	}
	else if( strcmp(request, "CWD") == 0)	// change working directory
	{
		chdir(currentUser -> currentPath);
		if( ( chdir(parameter) ) == 0 )
		{
			char path[PATH_LENGTH];
			getcwd(path, sizeof(path));
			strcpy(currentUser -> currentPath, path);
			reply(currentUser -> controlSocket, OPERATE_SUCCESS,	\
					"Change working directory success!");
		}
		else
		{
			reply(currentUser -> controlSocket, OPERATE_FAILED,	\
					"Change working directory failure!");
		}
		
	}
	else if( strcmp(request, "PWD") == 0)	// print working directory
	{
		char message[BUFFER_SIZE];
		sprintf(message, "\"%s\"", currentUser -> currentPath);
		reply(currentUser -> controlSocket, MKD_SUCCESS, message);
	}
	else if( strcmp(request, "LIST") == 0)	// list files amd directories
	{
		pid_t pid = fork();
		if(pid < 0)
		{
			fprintf(stderr, "fork failed\n");
			exit(-1);
		}
		else if(pid != 0)
		{
			// parent
			close(currentUser -> dataSocket);
			currentUser -> dataSocket = 0;
			return;
		}

		int fd = acceptNew( currentUser -> dataSocket);
			
		reply(currentUser -> controlSocket, STARTING_DATA,	\
				"here comes listing");

		DIR *dp;
		struct dirent *dirp;
		dp = opendir(currentUser -> currentPath);
		char buffer[BUFFER_SIZE] = {0};
		int n = 0;
		while( (dirp = readdir(dp)) != NULL)
		{
			sprintf(buffer, "%-25s", dirp -> d_name);

			if( n % 3 == 0)
			{
				strcat(buffer, "\r\n");
			}
			write(fd, buffer, strlen(buffer));
			n++;
		}
		sprintf(buffer, "\r\n");
		write(fd, buffer, strlen(buffer));

		close(fd);
		reply(currentUser -> controlSocket, CLOSING_DATA,	\
				"directory send OK");
		exit(0);
	}
	else if( strcmp(request, "RNFR") == 0)	// rename from
	{
		chdir(currentUser -> currentPath);		
		if( (access(parameter, F_OK)) == 0)
		{
			strcpy(currentUser -> renameFile, parameter);
			reply(currentUser -> controlSocket, FURTHER_INFO,	\
					"ready for RNTO");
		}
		else
			reply(currentUser -> controlSocket, OPERATE_FAILED,	\
					"RNFR command failed");
	}
	else if( strcmp(request, "RNTO") == 0)	// rename to
	{
		chdir(currentUser -> currentPath);
		if( rename(currentUser -> renameFile, parameter) == 0)
		{
			reply(currentUser -> controlSocket, OPERATE_SUCCESS,	\
					"rename successful");
		}
		else
			reply(currentUser -> controlSocket, OPERATE_FAILED,		\
					"rename failed");
	}
	else if( strcmp(request, "PASV") == 0)	// passive mode
	{
		struct sockaddr_in newServer;
		memset(&newServer, 0, sizeof(newServer));
		newServer.sin_family = AF_INET;
		// FIXME may have security issues
		newServer.sin_addr.s_addr = htonl(INADDR_ANY);
		newServer.sin_port = htons(UNUSED_PORT);

		int newServerfd = startServer(&newServer);

		struct sockaddr_in localAddressOfControlSocket;
		struct sockaddr_in localAddressOfNewServer;

		socklen_t addressLength;
		addressLength = sizeof(localAddressOfControlSocket);
		getsockname(currentUser -> controlSocket,	\
				(struct sockaddr*)&localAddressOfControlSocket,	\
						&addressLength);
		addressLength = sizeof(localAddressOfNewServer);
		getsockname(newServerfd,	\
				(struct sockaddr*)&localAddressOfNewServer,	\
					&addressLength);

		char message[BUFFER_SIZE] = {0};
		char addressBuffer[BUFFER_SIZE] = {0};
		int localPort = ntohs(localAddressOfNewServer.sin_port);
		sprintf(addressBuffer,"%s", inet_ntoa(	\
					localAddressOfControlSocket.sin_addr));
		int i;
		for(i = 0; i < strlen(addressBuffer); i++)
			if(addressBuffer[i] == '.')
				addressBuffer[i] = ',';

		sprintf(message, "entering passive mode (%s,%d,%d).",	\
				addressBuffer, localPort / 256, localPort % 256);
		reply(currentUser -> controlSocket, ENTER_PASSIVE, message);
		
		currentUser -> dataSocket = newServerfd;
		currentUser -> ftpMode = PASSIVE_MODE;

		if(DEBUG)
		{
			printf("local address: %s\n", \
					inet_ntoa( localAddressOfControlSocket.sin_addr) );
			printf("local port: %d\n", (int) ntohs(	\
						localAddressOfNewServer.sin_port));
		}
	}
	else if( strcmp(request, "TYPE") == 0)	// data transfer type
	{
		switch(parameter[0])
		{
			case 'I':
				reply(currentUser -> controlSocket, COMMAND_OK, \
						"always binary mode");
				break;
			default:
				reply(currentUser -> controlSocket, SYNTAX_ERROR, \
						"unrecognised TYPE command");
		}		
	}
	else if( strcmp(request, "SIZE") == 0)	// file size
	{
		chdir(currentUser -> currentPath);
		struct stat fileStatus;
		if( stat(parameter, &fileStatus) == 0)
		{
			char message[BUFFER_SIZE];
			sprintf(message, "%d", (int)fileStatus.st_size);
			reply(currentUser -> controlSocket, FILE_STATUS, message);
		}
		else
			reply(currentUser -> controlSocket, OPERATE_FAILED,	\
					"could not get file size");
	}
	else if( strcmp(request, "RETR") == 0)	// retrieve file
	{
		pid_t pid = fork();
		if(pid < 0)
		{
			fprintf(stderr, "fork failed\n");
			exit(-1);
		}
		else if(pid != 0)	// parent
			return;

		int datafd = acceptNew( currentUser -> dataSocket);

		chdir(currentUser -> currentPath);	
		int filefd;
		if( (filefd = open(parameter, O_RDONLY)) < 0)
		{
			reply(currentUser -> controlSocket, OPERATE_FAILED,	\
					"failed to open file");
			exit(0);
		}
		else
			reply(currentUser -> controlSocket, STARTING_DATA,	\
					"starting transfer file");

		int size;
		char buffer[BUFFER_SIZE];
		while( size = read(filefd, buffer, BUFFER_SIZE))
		{
			if( size < 0)
				{
					fprintf(stderr, "read error\n");
					exit(-1);
				}

			write(datafd, buffer, size);
		}
		reply(currentUser -> controlSocket, CLOSING_DATA,	\
				"file transfer complete");
		exit(0);
	}
	else if( strcmp(request, "STOR") == 0)	// store file
	{
		pid_t pid = fork();
		if(pid < 0)
		{
			fprintf(stderr, "fork failed\n");
			exit(-1);
		}
		else if(pid != 0)	// parent
			return;

		int datafd = acceptNew( currentUser -> dataSocket);

		chdir(currentUser -> currentPath);	
		int filefd;
		if( (filefd = open(parameter, O_WRONLY | O_CREAT | O_TRUNC, \
						FILE_MASK)) < 0)
		{
			reply(currentUser -> controlSocket, OPERATE_FAILED,	\
					"failed to open file");
			exit(0);
		}
		else
			reply(currentUser -> controlSocket, STARTING_DATA,	\
					"OK to receive data");
		
		int size;
		char buffer[BUFFER_SIZE];
		while( size = read(datafd, buffer, BUFFER_SIZE))
		{
			if( size < 0)
				{
					fprintf(stderr, "read error\n");
					exit(-1);
				}

			write(filefd, buffer, size);
		}
				
		reply(currentUser -> controlSocket, CLOSING_DATA,	\
				"file transfer complete");

		exit(0);
	}
	else if( strcmp(request, "DELE") == 0 ||	\
			 strcmp(request, "RMD") == 0)	// delete file or directory
	{
		chdir(currentUser -> currentPath);

		if((access(parameter, F_OK)) == 0)
		{ 
			if(remove(parameter) == 0)
				reply(currentUser -> controlSocket, OPERATE_SUCCESS, \
						"delete success!");
			else
				reply(currentUser -> controlSocket, OPERATE_FAILED,	\
						"delete failure!");
		}
		else
			reply(currentUser -> controlSocket, OPERATE_FAILED,	\
					"No such file or directory!");
	}
	else if( strcmp(request, "MKD") == 0)	// make directory
	{
		chdir(currentUser -> currentPath);

		if(mkdir(parameter,DIR_MASK) == 0) 
			reply(currentUser -> controlSocket, MKD_SUCCESS,	\
					"mkdir success");
		else
			reply(currentUser -> controlSocket, OPERATE_FAILED,	\
					"mkdir failure");
	}
	else if( strcmp(request, "QUIT") == 0)	// client logout
	{
		reply(currentUser -> controlSocket, SERVIVE_CLOSE, "goodbye");
		memset(currentUser -> currentPath, 0, PATH_LENGTH);
		removeSocket(currentUser -> controlSocket);
		currentUser -> controlSocket = 0;
		moveUser(&connectedUser, &unconnectedUser, currentUser);
		printf("user %s logged out\n", currentUser -> username);
	}
	else
	{
		reply(currentUser -> controlSocket, NOT_IMPLEMENT, \
				"command not implemented");
	}

	return;
}

void reply(int socketFileDescriptor, int replyCode, const char* message)
{
	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%d %s\r\n", replyCode, message);
	write(socketFileDescriptor, buffer, strlen(buffer));
}

int startServer(struct sockaddr_in* address)
{
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socketfd < 0)
	{
		fprintf(stderr, "error creating socket\n");
		exit(-1);
	}

	if( bind(socketfd, (struct sockaddr *) address, \
				sizeof(struct sockaddr_in)) < 0 )
	{
		fprintf(stderr, "can't bind to port %d\n", LISTEN_PORT);
		exit(-1);
	}

	if( listen(socketfd, LISTEN_QUEUE) < 0)
	{
		fprintf(stderr, "error converting to passive socket\n");
		exit(-1);
	}
	return socketfd;
}

void removeSocket(int fileDescriptor)
{
	close(fileDescriptor);
	FD_CLR(fileDescriptor, &readSet);

	if(fileDescriptor == maxFileDescriptor)
		maxFileDescriptor--;
}

static int acceptNew(int listenSocketFileDescriptor)
{
	struct sockaddr dataSocketAddress;
	socklen_t len = sizeof(dataSocketAddress);
	int fd = accept( listenSocketFileDescriptor, \
			(struct sockaddr *) &dataSocketAddress, \
			&len);
	return fd;
}
