#pragma once

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <ctime>
#include <io.h>

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

// winsock��ؿ�
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// _beginthread, _endthread
#include <process.h>
#include "cJSON.h"
#include "sqlite3.h"

#pragma comment (lib, "Ws2_32.lib")

#define CLIENT_BUFLEN 1024
#define MAX_CLIENT 1
#define PORT_CLIENT "9017"
#define MAX_DATANODE 10
#define PORT_DATANODE "9018"
#define PARAMFILE "params"
#define FILETREEFILE "filetree"

typedef struct CMDmsg
{
	int type;
	char msg[1020];
} CMDmsg;

typedef struct DataNode
{
	int status;
	DataNode *next;
	SOCKET dnsocket;
	char ip[17];
	// struct addrinfo *dnsocketresult;
	// struct addrinfo dnsockethints;
} DataNode;

int initialize();
void getCurrentTime();

// Client��غ���
void acceptClientIncoming(void *ListenSocket);
void processClient(void *index);
int prepareClientSocket();
void shutdownClientSocket();

// dnip��غ���
int loadparams();
int addparam(char ip[], int status);
int deleteparam(char ip[]);
void clearparam();
int saveparam();

int fetchparam(char ip[], DataNode *&returndn);
int connectEachDataNodes();
void connectSingleDataNode(void * curdn);
int disconnOnlineDataNodes();
int disconnectSingleDataNode(DataNode * &curdn);

int sendClientPackage(CMDmsg *msg, int index);
int sendDNPackage(SOCKET &dnsocket, CMDmsg *msg);

int loadFileTree();
int saveFileTree();

const char * GetEventMessage(DWORD dwCtrlType);
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);

void addtofilesDB(char *& filename, char *& sha256);
void addtoblocksDB(char *& blockdigest, char *& whichblocknum_str, char *& duplicatenum_str, char *& filedigest, char *& datanodeip);