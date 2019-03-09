#pragma once

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#undef UNICODE

// ȷ��ʹ��winsock1.1�Ĳ��ּ�����
#define WIN32_LEAN_AND_MEAN
// ��Ҫ�İ�
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// �˿ںͻ�������С
#include <process.h>
#include "cJSON.h"
#include "sqlite3.h"

#pragma comment (lib, "Ws2_32.lib")

#include "include/mbedtls/config.h"
#include "include/mbedtls/platform.h"
#define mbedtls_printf     printf
#include "include/mbedtls/sha256.h"

#define DEFAULT_BUFLEN 1024
#define PORT_SERVER "9017"
#define BLOCKDUPNUM 3

typedef struct CMDmsg
{
	int type;
	char msg[1020];
} CMDmsg;

typedef struct DNBlock
{
	char block[64 * 1024 * 1024];
};

const char * GetEventMessage(DWORD dwCtrlType);
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType);
int shutdownSocket();
void keepHeartBeat(void*);
int prepareSocket(int argc, char ** argv);
void keepHeartBeat(void *);
int shutdownSocket();
int prepareSocket(int argc, char ** argv);
int loadParameters();
int sendServerPackage(CMDmsg *msg);
int recvServerPackage(CMDmsg *recvpkg);

int buildConntoDN(char *ip, SOCKET DnConnectSocket);
int sendDNPackage(CMDmsg *msg, SOCKET targetSocket);
int recvDNPackage(CMDmsg *recvpkg, SOCKET targetSocket);




