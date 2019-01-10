#pragma once

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <ctime>
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
// winsock相关库
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// _beginthread, _endthread
#include <process.h>
#include "cJSON.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define CLIENT_BUFLEN 1024
#define MAX_CLIENT 1
#define PORT_CLIENT "27015"
#define MAX_DATANODE 10
#define PORT_DATANODE "27016"
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
int connectSingleDataNode(DataNode * &curdn);
int disconnOnlineDataNodes();
int disconnectSingleDataNode(DataNode * &curdn);

int sendClientPackage(CMDmsg *msg, int index);
int sendDNPackage(SOCKET &dnsocket, CMDmsg *msg);

int loadFileTree();
int saveFileTree();

const char * GetEventMessage(DWORD dwCtrlType);
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);