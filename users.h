#ifndef USERS_H
#define USERS_H

#include <time.h>

#define NAME_LENGTH		64
#define PATH_LENGTH		1024

#define ACTIVE_MODE		0
#define PASSIVE_MODE	1

#define DISABLE			0
#define ENABLE			1

struct userInformation
{
	char username[NAME_LENGTH];		
	char password[NAME_LENGTH];
	char homeDirectory[PATH_LENGTH];// full path of user's home directory
	char currentPath[PATH_LENGTH];	// full path of current working directory
	int writeAccess;				// ENABLE or DISABLE 
	int readAccess;					// ENABLE or DISABLE
	char renameFile[PATH_LENGTH];	// file to be renamed
	int controlSocket;				// control connection file descriptor
	int	dataSocket;					// data-transfer listen socket file 
									// descriptor, used in passive mode
	int dataPortNumber;				// port number for data transmission,
									// used in active mode	
	int ftpMode;					// ACTIVE_MODE or PASSIVE_MODE
	time_t loginTime;				// when the user logged in
	long long dataTransferred;		// total bytes of data transferred
	int speedLimit;					// the speed limit of uploading and 
									// downloading, in kB/s. 0 if no limit
	
	// stored as linked list, so:
	struct userInformation* previous;
	struct userInformation* next;
};

typedef struct userInformation user;

struct userlist
{
	user* first;
	user* last;
};

typedef struct userlist userList;

user* addNewUser(userList* list);
void moveUser(userList* from, userList* to, user* theUser);
user* findUserByName(const char* name, userList* list);
user* findUserByControlSocketFileDescriptor(int fileDescriptor, userList* list);

void printUserList(userList* list);

#endif
