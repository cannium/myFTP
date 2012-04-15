#ifndef COMMAND_H
#define COMMAND_H

#include "users.h"

// reply codes
#define STARTING_DATA	150
#define SERVICE_READY	220
#define SERVIVE_CLOSE	221
#define CLOSING_DATA	226
#define ENTER_PASSIVE	227
#define LOGIN_SUCCESS	230
#define OPERATE_SUCCESS	250
#define MKD_SUCCESS		257
#define NEED_PASSWORD	331
#define NOT_IMPLEMENT	502
#define LOGIN_FAILED	530
#define OPERATE_FAILED	550

void handleCommand(user* currentUser, const char* buffer, ssize_t size);
void reply(int socketFileDescriptor, int replyCode, const char* message);
int startServer(struct sockaddr_in* address);
void removeSocket(int fileDescriptor);

#endif
