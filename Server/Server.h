#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include <ctime>
#include <io.h>

#undef UNICODE
#define WIN32_LEAN_AND_MEAN

// winsock相关库
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// _beginthread, _endthread
#include <process.h>
#include "sqlite3.h"
#include "cJSON.h"

#pragma comment (lib, "Ws2_32.lib")

#define CLIENT_BUFLEN 1024
#define DN_CMD_BUFLEN 1024
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
} DataNode;

int initialize();
void getCurrentTime();

// Client相关函数
void acceptClientIncoming(void *ListenSocket);
void processClient(void *index);
int prepareClientSocket();
void shutdownClientSocket();

// dnip相关函数
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

int recvDNCMDmsg(CMDmsg *recvpkg, SOCKET &targetSocket);

int loadFileTree();
int saveFileTree();

const char * GetEventMessage(DWORD dwCtrlType);
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);

bool addtofilesDB(char *& filename, char *& blocksumnum, char *& duplicatenum, char *& sha256);
bool addtoblocksDB(char *& blockdigest, char *& whichblocknum_str,
	char *& duplicatenum_str, char *& filedigest, char *& datanodeip);
void clear_dnip_blockhash();
int callback_blocksumnum(void *data, int argc, char **argv, char **azColName);
int getfiledb_blocksumnum(char * blockhash);
int callback_blocklocation(void *data, int argc, char **argv, char **azColName);
int getblocksdb_blockdigest_dnip(char * filehash, char * blockseriesnum, char * duplicatenum);