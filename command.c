#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "global.h"
#include "command.h"

void handleCommand(user* currentUser, const char* buffer, ssize_t size)
{
	printf("handling user %s : %s\n", currentUser -> username, buffer);

	pid_t pid = fork();
	if(pid < 0)
	{
		fprintf(stderr, "fork failed\n");
		exit(-1);
	}
	else if(pid != 0)	// parent
		return;

	char request[REQUEST_BUFF], parameter[BUFFER_SIZE];
	sscanf(buffer, "%4s %1023s", request, parameter);

	if( strcmp(request, "PASS") == 0)		// password
	{
		
	}
	else if( strcmp(request, "CWD") == 0)	// change working directory
	{
		
	}
	else if( strcmp(request, "LIST") == 0)	// list files amd directories
	{
		
	}
	else if( strcmp(request, "RNFR") == 0)	// rename from
	{
		
	}
	else if( strcmp(request, "RNTO") == 0)	// rename to
	{
		
	}
	else if( strcmp(request, "PASV") == 0)	// passive mode
	{
		
	}
	else if( strcmp(request, "RETR") == 0)	// retrieve file
	{
		
	}
	else if( strcmp(request, "STOR") == 0)	// store file
	{
		
	}
	else if( strcmp(request, "DELE") == 0)	// delete file
	{
		
	}
	else if( strcmp(request, "MKD") == 0)	// make directory
	{
		
	}
	else if( strcmp(request, "QUIT") == 0)	// client logout
	{
		
	}
	else
	{
		reply(currentUser -> controlSocket, NOT_IMPLEMENT, \
				"command not implemented");
	}

	exit(0);
}

void reply(int socketFileDescriptor, int replyCode, const char* message)
{
	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%d %s\n", replyCode, message);
	write(socketFileDescriptor, buffer, strlen(buffer));
}

