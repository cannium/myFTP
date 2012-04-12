#include <stdio.h>
#include <string.h>

#include "global.h"
#include "command.h"

void handleCommand(user* currentUser, const char* buffer, ssize_t size)
// handles PASS, CWD, LIST, MDIR, DELE, RNFR/RNTO, QUIT
{
	printf("handling user %s : %s\n", currentUser -> username, buffer);
	return;
}

void reply(int socketFileDescriptor, int replyCode, const char* message)
{
	char buffer[BUFFER_SIZE];
	sprintf(buffer, "%d %s\n", replyCode, message);
	write(socketFileDescriptor, buffer, strlen(buffer));
}

int getRequestCodeLen(const char* request, int total)
{
	int len;
	for(len = 0; len < total; len++)
	{
		if(request[len] == ' ')
		{
			return len;
		}
	}
	return total;
}
