#ifndef COMMAND_H
#define COMMAND_H

// reply codes
#define SERVICE_READY	220
#define SERVIVE_CLOSE	221
#define LOGIN_SUCCESS	230
#define NEED_PASSWORD	331
#define LOGIN_FAILED	530

void handleCommand(char* buffer, ssize_t size);

#endif
