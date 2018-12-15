// 自动生成
#include "pch.h"
#include <iostream>

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 512
#define MAX_CLIENT 4
#define PORT_HEARTBEAT "27015"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>    /* _beginthread, _endthread */

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

int    freeclientportnum = MAX_CLIENT;
HANDLE ClientThread[MAX_CLIENT];
SOCKET ClientSocket[MAX_CLIENT];
int	   ClientSocketStatus[MAX_CLIENT];
BOOL g_exit = FALSE;
BOOL all_closed = FALSE;

void acceptClientIncoming(void *ListenSocket);
void heartbeatClient(void *index);


const char * GetEventMessage(DWORD dwCtrlType) {
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

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
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

int main(void) {
	if (SetConsoleCtrlHandler(HandlerRoutine, TRUE)) {
	}
	else {
		printf("error setting handler\n");
		return 1;
	}

	printf("Server starting ...\n");

	WSADATA wsaData;
	int iResult;
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	SOCKET ListenSocket = INVALID_SOCKET;
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// 对于数组的初始化，这种写法可能有兼容性问题，有的编译器不一定支持这种写法
	// SOCKET ClientSocket[MAX_CLIENT];
	// ClientSocket[MAX_CLIENT] = { INVALID_SOCKET };
	memset(ClientThread, NULL, MAX_CLIENT * sizeof(HANDLE));
	memset(ClientSocket, INVALID_SOCKET, MAX_CLIENT * sizeof(SOCKET));
	memset(ClientSocketStatus, 0, MAX_CLIENT * sizeof(int));

	// 1. 初始化Socket，无论是C还是S都要有这一步，否则会出10083错误代码
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	// 2. 创建一个socket
	// 指定地址结构，表示要选择的下一层协议类型等参数
	// 关于hint（sockaddr）结构体的说明：
	// https ://docs.microsoft.com/zh-cn/windows/desktop/WinSock/sockaddr-2
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	// 尝试根据端口解析，检查物理端口可不可用、前面的sockaddr地址结构有没有错误等等
	iResult = getaddrinfo(NULL, PORT_HEARTBEAT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	// 根据物理参数创建一个监听Socket
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// 3. 绑定一个监听socket到物理socket上，result就是物理socket的一些参数
	// 绑定监听socket到物理socket上
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	// 所谓的物理socket，其实就是创建socket对象的一个中介信息，当创建socket对象的行为顺利完成，
	// 也就不需要前面准备的物理socket信息了
	// 所以这里可以释放掉这个"物理的"socket address info
	freeaddrinfo(result);

	// 4. 开启监听接口，至此初始化完成
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("initialized iResult is %d.\n", iResult);

	// 启动监听Client进入的线程
	HANDLE handle_incoming = (HANDLE)_beginthread(acceptClientIncoming, 0, (void *)ListenSocket);
	// HANDLE handle_process = (HANDLE)_beginthread(processClient, 0, NULL);

	// 6. 接收或发送消息
	do {
		// 5. 尝试接受Client的连接请求，并对ListenSocket传来的请求接收成为ClientSocket
		//clientsocket[freesocketindex] = accept(listensocket, null, null);
		//if (clientsocket[freesocketindex] == invalid_socket) {
		//	printf("accept failed with error: %d\n", wsagetlasterror());
		//	// closesocket(listensocket);
		//	// wsacleanup();
		//	continue;
		//}
		//iResult = recv(ClientSocket[freesocketindex], recvbuf, recvbuflen, 0);
		//printf("message received: %s", recvbuf);
		//if (iResult > 0) {
		//	printf("Bytes received: %d\n", iResult);
		//	ZeroMemory(recvbuf, sizeof(recvbuf));
		//	// 回复发送方
		//	iSendResult = send(ClientSocket[freesocketindex], recvbuf, iResult, 0);
		//	if (iSendResult == SOCKET_ERROR) {
		//		printf("send failed with error: %d\n", WSAGetLastError());
		//		closesocket(ClientSocket[freesocketindex]);
		//		WSACleanup();
		//		return 1;
		//	}
		//	printf("Bytes sent: %d\n", iSendResult);
		//	iResult = -1;
		//}
		//else if (iResult == 0) {
		//	printf("Connection closing...\n");
		//	iResult = -1;
		//} else {
		//	printf("A client just aborted.");
		//	// 不再需要服务器时socket可以关闭
		//	closesocket(ListenSocket);
		//	iResult = -1;
		//}
	} while (!g_exit);
	// } while (iResult > 0);
	printf("Server closing...\n");
	printf("------------------------------------------\n");

	CloseHandle(handle_incoming);
	for (int k = 0; k < MAX_CLIENT; k++) {
		if (ClientSocketStatus[k] != 0) {
			ClientSocketStatus[k] = 0;
			CloseHandle(ClientThread[k]);
			int iResult = shutdown(ClientSocket[k], SD_SEND);
			if (iResult == SOCKET_ERROR) {
				printf("shutdown failed with error: %d\n", WSAGetLastError());
			}
			closesocket(ClientSocket[k]);
		}
	}

	WSACleanup();

	printf("------------------------------------------\n");
	printf("Server Closed.\n");
	printf("press any key to exit.\n");
	getchar();

	return 0;
}

void acceptClientIncoming(void *ListenSocket)
{
	do {
		if (freeclientportnum == 0) continue;

		int i = 0;
		for (;; i = (i + 1) % MAX_CLIENT) {
			if (ClientSocketStatus[i] == 0) break;
		}

		ClientSocket[i] = accept((SOCKET)ListenSocket, NULL, NULL);
		if (ClientSocket[i] == INVALID_SOCKET) {
			ClientSocketStatus[i] = 0;
			printf("Client accept failed with error: %d\n", WSAGetLastError());
		}
		else {
			freeclientportnum--;
			ClientSocketStatus[i] = 1;
			ClientThread[i] = (HANDLE)_beginthread(heartbeatClient, 0, (void *)i);
			printf("%d Client accept success.\n", i);
		}
	} while (!g_exit);
}

void heartbeatClient(void *index)
{
	int myindex = (int)index;
	char recvbuf[DEFAULT_BUFLEN];
	ZeroMemory(recvbuf, sizeof(recvbuf));
	int recvbuflen = DEFAULT_BUFLEN;

	do {
		if (ClientSocketStatus[myindex] == 0)
			continue;
		int iResult;
		// 这么写不行，前面端口的接收会不断地阻塞这个for，阻止后面的端口继续接收消息
		// 所以应该给每个Client都启动一个thread
		iResult = recv(ClientSocket[myindex], recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("------------------------------\n");
			printf("%d Client:", myindex);
			printf("Bytes received: %d\n", iResult);
			printf("Message received: %s\n", recvbuf);
			printf("------------------------------\n");
			ZeroMemory(recvbuf, sizeof(recvbuf));
			// 回复发送方
			//iSendResult = send(ClientSocket[freesocketindex], recvbuf, iResult, 0);
			//if (iSendResult == SOCKET_ERROR) {
			//	printf("send failed with error: %d\n", WSAGetLastError());
			//	closesocket(ClientSocket[freesocketindex]);
			//	WSACleanup();
			//	return 1;
			//}
			//printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0) {
			printf("%d Client Connection closing...\n", myindex);
			ClientSocketStatus[myindex] = 0;
			CloseHandle(ClientThread[myindex]);
			closesocket(ClientSocket[myindex]);
			freeclientportnum++;
			printf("%d Client just shut down.\n", myindex);
		}
		else {
			printf("$d Client Connection closing...\n", myindex);
			ClientSocketStatus[myindex] = 0;
			CloseHandle(ClientThread[myindex]);
			iResult = shutdown(ClientSocket[myindex], SD_SEND);
			if (iResult == SOCKET_ERROR) {
				printf("$d Client shutdown failed with error: %d\n", myindex, WSAGetLastError());
			}
			closesocket(ClientSocket[myindex]);
			freeclientportnum++;
			printf("%d Client just shut down.\n", myindex);
		}
	} while (!g_exit);
}
