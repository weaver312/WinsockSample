#include <iostream>
// 确保使用winsock1.1的部分兼容性
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// 端口和缓冲区大小
#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "27015"
#define DNFILENAME "params"

// 必要的包
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <stdio.h>
// _beginthread, _endthread
#include <process.h>

// 需要链接的本地库
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

typedef struct CMDmsg
{
	int type;
	char msg[1020];
} CMDmsg;

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