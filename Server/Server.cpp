#include "pch.h"
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
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define CLIENT_BUFLEN 1024
#define MAX_CLIENT 1
#define PORT_CLIENT "27015"
#define MAX_DATANODE 6
#define PORT_DATANODE "27016"
#define PARAMFILE "params"

WSADATA wsaData;
BOOL g_exit = FALSE;
char CTIME[24];
int iResult = 0;

// Client相关变量
int    freeclientportnum = MAX_CLIENT;
HANDLE ClientThread[MAX_CLIENT];
SOCKET ClientSocket[MAX_CLIENT];
int	   ClientSocketStatus[MAX_CLIENT];
int clientiResult;
struct addrinfo *clientresult = NULL;
struct addrinfo clienthints;
SOCKET ClientListenSocket = INVALID_SOCKET;
HANDLE handle_incoming;

// Client消息结构体
typedef struct ClientMessage
{
	int type;
	char msg[1020];
} ClientMessage;
// 存储dn的链表节点
typedef struct Datanode
{
	char ip[17];
	int status;
	Datanode *next;
} Datanode;

// DATANODE相关变量
int DN_alive = 0;
Datanode *datanodes;

int initialize();
void getCurrentTime();
// 截获键盘消息函数
const char * GetEventMessage(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	// 0，按下了ctrl+c，或者通过GenerateConsoleCtrlEvent函数产生信号
	case CTRL_C_EVENT:
		return "CTRL_C_EVENT";

	// 1，按下ctrl+break，或通过函数产生信号
	case CTRL_BREAK_EVENT:
		return "CTRL_BREAK_EVENT";

	// 2，按下GUI的关闭console按钮
	case CTRL_CLOSE_EVENT:
		return "CTRL_CLOSE_EVENT";

	// 5，WIndows用户注销
	case CTRL_LOGOFF_EVENT:
		return "CTRL_LOGOFF_EVENT";

	// 6，用户关闭了Windows系统
	case CTRL_SHUTDOWN_EVENT:
		return "CTRL_SHUTDOWN_EVENT";
	}

	return "Unknown";
}
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	// 关于这个数的类型参照：
	// whttps://docs.microsoft.com/zh-cn/windows/console/handlerroutine
	printf("%s event received\n", GetEventMessage(dwCtrlType));
	// 简单处理，对上述情况都会启动关闭各个客户端这一流程
	if (dwCtrlType >= 0)
		g_exit = TRUE;
	// 如果return FALSE，操作系统就会让下一个Handler继续处理
	// 如果return TRUE，就会截获这个消息，不再继续传递
	return TRUE;
}

int main(void);

// Client相关函数
void acceptClientIncoming(void *ListenSocket);
void processClient(void *index);
int prepareClientSocket();
void shutdownClientSocket();

int loadparams();
int addparam(char ip[], int status);
int deleteparam(char ip[]);
void clearparam();
int saveparam();

/*
对服务器来说，一个socket应用通常有以下环节：
1. 初始化Winsock
2. 创建物理socket（ip + port）
2. 根据物理socket创建一个socket对象
3. 绑定socket对象到物理socket
4. 监听Client
5. 接受Client的连接
6. 接收消息/发送消息
7. 断开连接，释放资源
*/

int main(void)
{
	getCurrentTime();
	printf("%s Server starting ...\n", CTIME);

	iResult = initialize();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Initialize error!", CTIME);
		return 1;
	}
	iResult = prepareClientSocket();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s PrepareClientSocket error!", CTIME);
		return 1;
	}
	iResult = loadparams();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading datanodes!", CTIME);
		return 1;
	}

	// 启动监听Client的线程
	handle_incoming = (HANDLE)_beginthread(acceptClientIncoming, 0, (void *)ClientListenSocket);

	int dniResult;
	struct addrinfo dnhints;
	struct addrinfo *dnresult = NULL;
	struct addrinfo *dnptr = NULL;
	SOCKET DnListenSocket = INVALID_SOCKET;

	int i = 0;
	/*
	for (; i < 4; i++) {
		ZeroMemory(&dnhints, sizeof(dnhints));
		dnhints.ai_family = AF_UNSPEC;
		dnhints.ai_socktype = SOCK_STREAM;
		dnhints.ai_protocol = IPPROTO_TCP;
		// 尝试根据端口解析，检查物理端口可不可用、前面的sockaddr地址结构有没有错误等等
		dniResult = getaddrinfo(dnlist[i], PORT_DATANODE, &dnhints, &dnresult);
		if (dniResult != 0) {
			getCurrentTime();
			printf("%s DNSocket get address info failed with error: %d\n", CTIME, dniResult);
			WSACleanup();
			return 1;
		}

		for (dnptr = dnresult; dnptr != NULL; dnptr = dnptr->ai_next) {
			DnListenSocket = socket(dnresult->ai_family, dnresult->ai_socktype, dnresult->ai_protocol);
			if (DnListenSocket == INVALID_SOCKET) {
			getCurrentTime();
			printf("%s DnListenSocket create failed with error: %ld\n", CTIME, WSAGetLastError());
			freeaddrinfo(dnresult);
			WSACleanup();
			return 1;
		}

			dniResult = connect(DnListenSocket, dnptr->ai_addr, (int)dnptr->ai_addrlen);
			if (dniResult == SOCKET_ERROR) {
				getCurrentTime();
				printf("%s DnListenSocket %d connect failed with error: %d\n", CTIME, i, WSAGetLastError());
				DnListenSocket = INVALID_SOCKET;
				continue;
			}
			break;
		}
		freeaddrinfo(dnresult);

		// 连接失败
		if (DnListenSocket == INVALID_SOCKET) {
			getCurrentTime();
			printf("%s Unable to connect to %d client!\n", CTIME, i);
			WSACleanup();
			return 1;
		}

		//dniResult = bind(DnListenSocket, dnresult->ai_addr, (int)dnresult->ai_addrlen);
		//freeaddrinfo(dnresult);
		//// 4. 开启监听接口，至此初始化完成
		//dniResult = listen(DnListenSocket, SOMAXCONN);
		//if (dniResult == SOCKET_ERROR) {
		//	getCurrentTime();
		//	printf("%s DnListenSocket listen failed with error: %d\n", CTIME, WSAGetLastError());
		//	closesocket(DnListenSocket);
		//	WSACleanup();
		//	return 1;
		//}
		//getCurrentTime();
		//printf("%s DnListenSocket initialized, dniResult is %d.\n", CTIME, dniResult);
		//return 0;
	}
	*/
	do { } while (!g_exit);

	getCurrentTime();
	printf("%s Server closing...\n", CTIME);
	printf("------------------------------------------\n");
	CloseHandle(handle_incoming);

	shutdownClientSocket();

	WSACleanup();
	printf("------------------------------------------\n");
	getCurrentTime();
	printf("%s Server closed, press any key to exit.\n", CTIME);
	getchar();

	return 0;
}

void acceptClientIncoming(void *ListenSocket)
{
	while (!g_exit) {
		if (freeclientportnum==0) continue;

		int i = 0;
		for (;; i = (i + 1) % MAX_CLIENT) {
			if (ClientSocketStatus[i] == 0) break;
		}

		ClientSocket[i] = accept((SOCKET)ListenSocket, 0, 0);
		if (ClientSocket[i] == INVALID_SOCKET) {
			ClientSocketStatus[i] = 0;
			getCurrentTime();
			printf("%s Client accept failed with error: %d\n", CTIME, WSAGetLastError());
		}
		else {
			freeclientportnum--;
			ClientSocketStatus[i] = 1;
			ClientThread[i] = (HANDLE)_beginthread(processClient, 0, (void *)i);
			getCurrentTime();
			printf("%s Client %d accept success.\n", CTIME, i);
		}
	}
}

void processClient(void *index)
{
	int myindex = (int)index;
	char recvbuf[CLIENT_BUFLEN];
	ZeroMemory(recvbuf, sizeof(recvbuf));
	int recvbuflen = CLIENT_BUFLEN;
	ClientMessage test;

	do {
		if (ClientSocketStatus[myindex] == 0)
			continue;
		int iResult;
		iResult = recv(ClientSocket[myindex], recvbuf, recvbuflen, 0);

		// 结构体各域不一定连续，要分开赋值
		char *currentAddr = recvbuf;
		memcpy_s(&test.type, sizeof(test.type), currentAddr, sizeof(test.type));
		currentAddr += sizeof(test.type);
		memcpy_s(test.msg, sizeof(test.msg), currentAddr, sizeof(test.msg));

		// If no byte received, current thread will be blocked in recv func.

		// If no error occurs, recv returns the number of bytes received and the buffer pointed to by the buf parameter will contain this data received.
		if (iResult > 0) {
			//printf("------------------------------\n");
			getCurrentTime();
			printf("%s %d Client HeartBeat Message received: %d - %s\n", CTIME, myindex, test.type , test.msg);
			//printf("------------------------------\n");
			ZeroMemory(recvbuf, sizeof(recvbuf));
			// 回复发送方
			//iSendResult = send(ClientSocket[freesocketindex], recvbuf, clientiResult, 0);
			//if (iSendResult == SOCKET_ERROR) {
			//	printf("send failed with error: %d\n", WSAGetLastError());
			//	closesocket(ClientSocket[freesocketindex]);
			//	WSACleanup();
			//	return 1;
			//}
			//printf("Bytes sent: %d\n", iSendResult);
		}
		// If the connection has been gracefully closed, the return value is zero.
		else if (iResult == 0) {
			getCurrentTime();
			printf("%s %d Client Connection closing...\n", CTIME, myindex);
			ClientSocketStatus[myindex] = 0;
			CloseHandle(ClientThread[myindex]);
			closesocket(ClientSocket[myindex]);
			freeclientportnum++;
			getCurrentTime();
			printf("%s %d Client just shut down.\n", CTIME, myindex);
		}
		// Otherwise, a value of SOCKET_ERROR is returned, and a specific error code can be retrieved by calling WSAGetLastError.
		else {
			getCurrentTime();
			printf("%s %d An Exception happened. iResult is %d and WSALastERROR is %d.\n", CTIME, myindex, iResult, WSAGetLastError());
			ClientSocketStatus[myindex] = 0;
			CloseHandle(ClientThread[myindex]);
			// If last error is greater than 10000, we should try to shut down the error socket.
			// If not, try to continue using ignoring the error.
			 iResult = shutdown(ClientSocket[myindex], SD_SEND);
			 if (iResult == SOCKET_ERROR) {
			 	 getCurrentTime();
			   printf("%s %d Client shutdown failed with error: %d\n", CTIME, myindex, WSAGetLastError());
			 }
			 closesocket(ClientSocket[myindex]);
			 freeclientportnum++;
			 getCurrentTime();
			 printf("%s %d Client just shut down.\n", CTIME, myindex);
		}
	} while (!g_exit);
}

int prepareClientSocket()
{
	// 2. 创建一个socket
	// 指定地址结构，表示要选择的下一层协议类型等参数
	// 关于hint（sockaddr）结构体的说明：
	// https ://docs.microsoft.com/zh-cn/windows/desktop/WinSock/sockaddr-2
	ZeroMemory(&clienthints, sizeof(clienthints));
	clienthints.ai_family = AF_INET;
	clienthints.ai_socktype = SOCK_STREAM;
	clienthints.ai_protocol = IPPROTO_TCP;
	clienthints.ai_flags = AI_PASSIVE;
	// 尝试根据端口解析，检查物理端口可不可用、前面的sockaddr地址结构有没有错误等等
	clientiResult = getaddrinfo(NULL, PORT_CLIENT, &clienthints, &clientresult);
	if (clientiResult != 0) {
		getCurrentTime();
		printf("%s ClientSocket get address info failed with error: %d\n", CTIME, clientiResult);
		WSACleanup();
		return 1;
	}
	// 根据物理参数创建一个监听Socket
	ClientListenSocket = socket(clientresult->ai_family, clientresult->ai_socktype, clientresult->ai_protocol);
	if (ClientListenSocket == INVALID_SOCKET) {
		getCurrentTime();
		printf("%s ClientSocket create failed with error: %ld\n", CTIME, WSAGetLastError());
		freeaddrinfo(clientresult);
		WSACleanup();
		return 1;
	}

	// 3. 绑定一个监听socket到物理socket上，clientresult就是物理socket的一些参数
	// 绑定监听socket到物理socket上
	clientiResult = bind(ClientListenSocket, clientresult->ai_addr, (int)clientresult->ai_addrlen);
	if (clientiResult == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s ClientSocket bind failed with error: %d\n", CTIME, WSAGetLastError());
		freeaddrinfo(clientresult);
		closesocket(ClientListenSocket);
		WSACleanup();
		return 1;
	}
	// 所谓的物理socket，其实就是创建socket对象的一个中介信息，当创建socket对象的行为顺利完成，
	// 也就不需要前面准备的物理socket信息了
	// 所以这里可以释放掉这个"物理的"socket address info
	freeaddrinfo(clientresult);

	// 4. 开启监听接口，至此初始化完成
	clientiResult = listen(ClientListenSocket, SOMAXCONN);
	if (clientiResult == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s ClientSocket listen failed with error: %d\n", CTIME, WSAGetLastError());
		closesocket(ClientListenSocket);
		WSACleanup();
		return 1;
	}
	getCurrentTime();
	printf("%s ClientSocket initialized, clientiResult is %d.\n", CTIME, clientiResult);
	return 0;
}

void shutdownClientSocket()
{
	for (int k = 0; k < MAX_CLIENT; k++) {
		if (ClientSocketStatus[k] != 0) {
			ClientSocketStatus[k] = 0;
			CloseHandle(ClientThread[k]);
			int iResult = shutdown(ClientSocket[k], SD_SEND);
			if (iResult == SOCKET_ERROR) {
				getCurrentTime();
				printf("%s shutdown failed with error: %d\n", CTIME, WSAGetLastError());
			}
			closesocket(ClientSocket[k]);
		}
	}
}

int loadparams()
{
	getCurrentTime();
	printf("%s Loading parameters\n", CTIME);

	datanodes = (Datanode *)malloc(sizeof(Datanode));
	Datanode *tempnode = datanodes;

	FILE *paramsfp = NULL;
	errno_t err;
	// 为了首次使用能自动创建配置文件，这里用w+参数为好
	err = fopen_s(&paramsfp, PARAMFILE, "r");
	if (err != 0) return 1;
	if (paramsfp == NULL) {
		fclose(paramsfp);
		paramsfp = NULL;
		fopen_s(&paramsfp, PARAMFILE, "w+");
		return 1;
	}
	else {
		char ipline[17];
		while (!feof(paramsfp)) {
			if (fgets(ipline, 17, paramsfp) == NULL)
				break;
			printf("%s", ipline);
			strcpy_s(tempnode->ip, 17, ipline);
			tempnode->next = (Datanode *)malloc(sizeof(Datanode));
			tempnode->status = 0;
			tempnode = tempnode->next;
		}
	}
	printf("\n");
	fclose(paramsfp);

	getCurrentTime();
	printf("%s Loading parameters finished\n", CTIME);
	return 0;
}

int addparam(char ip[], int status) {
	// check if exists duplicate ip param
	Datanode *temp;
	temp = datanodes;
	while (temp != NULL) {
		if (strcmp(datanodes->ip, ip) == 0) {
			return 1;
		}
		temp = datanodes->next;
	}
	temp = NULL;

	temp = (Datanode *)malloc(sizeof(Datanode));
	strcpy_s(temp->ip, 17, ip);
	temp->status = status;
	if (datanodes->next != NULL) {
		temp->next = datanodes->next;
		datanodes->next = temp;
	}
	else {
		datanodes->next = temp;
	}
	return 0;
}

int deleteparam(char ip[]) {
	Datanode *pre = datanodes;
	Datanode *cur = datanodes;
	while (cur != NULL) {
		if (strcmp(datanodes->ip, ip) == 0) {
			pre->next = cur->next;
			return 0;
		}
		pre = cur;
		cur = cur->next;
	}
	return 1;
}

void clearparam() {
	datanodes = NULL;
	datanodes = (Datanode *)malloc(sizeof(Datanode));
}

int saveparam() {
	FILE *paramsfp = NULL;
	errno_t err;
	err = fopen_s(&paramsfp, PARAMFILE, "w");
	if (err != 0) return 1;
	if (paramsfp == NULL) {
		fclose(paramsfp);
		return 1;
	}
	Datanode *temp = datanodes;
	const char *nextline = "\n";
	while (temp != NULL && strlen(temp->ip) != 0) {
		fputs(temp->ip, paramsfp);
		fputs(nextline, paramsfp);
		temp = temp->next;
	}
	fclose(paramsfp);
	return 0;
}

// 初始化Client变量，设置键监听
int initialize()
{
	if (SetConsoleCtrlHandler(HandlerRoutine, TRUE)) {
	} else {
		getCurrentTime();
		printf("%s error setting handler\n", CTIME);
		return 1;
	}

	// 对于数组的初始化，这种写法可能有兼容性问题，有的编译器不一定支持这种写法
	// SOCKET ClientSocket[MAX_CLIENT];
	// ClientSocket[MAX_CLIENT] = { INVALID_SOCKET };
	memset(ClientThread, NULL, MAX_CLIENT * sizeof(HANDLE));
	memset(ClientSocket, INVALID_SOCKET, MAX_CLIENT * sizeof(SOCKET));
	memset(ClientSocketStatus, 0, MAX_CLIENT * sizeof(int));

	// 1. 初始化Socket，无论是C还是S都要有这一步，否则会出10083错误代码
	clientiResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (clientiResult != 0) {
		getCurrentTime();
		printf("%s WSAStartup failed with error: %d\n", CTIME, clientiResult);
		return 1;
	}
	return 0;
}

// 获取当前时刻字符串到CTIME变量中
void getCurrentTime()
{
	time_t currenttime;
	struct tm currentlocaltime;

	time(&currenttime);
	// 出于安全性，要求使用localtime_s，linux下使用localtime_r
	localtime_s(&currentlocaltime, &currenttime);
	// i 表示格式化字符串长度，具体例子见网，这里没用
	int i = snprintf(CTIME, sizeof(CTIME), "%02d-%02d-%02d %02d:%02d:%02d",
		currentlocaltime.tm_year + 1900,
		currentlocaltime.tm_mon + 1,
		currentlocaltime.tm_mday,
		currentlocaltime.tm_hour,
		currentlocaltime.tm_min,
		currentlocaltime.tm_sec);
}
