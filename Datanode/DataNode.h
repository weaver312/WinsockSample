#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <ctime>
#include <io.h>
#include <process.h>
#include <string.h>

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

// winsockœ‡πÿø‚
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "sqlite3.h"


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define SERVER_BUFLEN 1024
#define SERVER_PORT "9018"
#define CLIENT_BUFLEN 1024
#define CLIENT_PORT "9019"

typedef struct CMDmsg
{
	int type;
	char msg[1020];
} CMDmsg;

typedef struct DNBlock
{
	int dupnum;
	int blocknum;
	int blocklength;
	char sha256blockdigest[64];
	char sha256filedigest[64];
	char blockbinarycontent[64 * 1024 * 1024];
} DNBlock;

int init(char *& targetaddr);
void getCurrentTime();
void processServer(void *);
void processClient(void *);
void acceptServerIncoming(void *ListenSocket);
void acceptClientIncoming(void *ListenSocket);
int prepareServerSocket(char *& targetaddr);
int prepareClientSocket(char *& targetaddr);
int sendServerPackage(CMDmsg *msg);
bool addtoblocksDB(char *blockdigest, char * whichblocknum_str, char *blocklength,
	char *duplicatenum_str, char *filedigest);
int sendClientCMDmsg(CMDmsg *msg, SOCKET &socket);