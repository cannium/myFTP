#ifndef COMMAND_H
#define COMMAND_H

#include "users.h"

// reply codes
#define SERVICE_READY	220
#define SERVIVE_CLOSE	221
#define LOGIN_SUCCESS	230
#define NEED_PASSWORD	331
#define LOGIN_FAILED	530

void handleCommand(user* currentUser, const char* buffer, ssize_t size);
void reply(int socketFileDescriptor, int replyCode, const char* message);
int getRequestCodeLen(const char* request, int total);

#endif
