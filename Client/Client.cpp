// 自动生成
#include "pch.h"
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
#include "Client.h"
// _beginthread, _endthread
#include <process.h>

// 需要链接的本地库
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

bool g_exit = false;

typedef struct ServerMessage
{
	int type;
	char msg[1020];
} ServerMessage;

const char * GetEventMessage(DWORD dwCtrlType) {
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		return "CTRL_C_EVENT";

	case CTRL_BREAK_EVENT:
		return "CTRL_BREAK_EVENT";

	case CTRL_CLOSE_EVENT:
		return "CTRL_CLOSE_EVENT";

	case CTRL_LOGOFF_EVENT:
		return "CTRL_LOGOFF_EVENT";

	case CTRL_SHUTDOWN_EVENT:
		return "CTRL_SHUTDOWN_EVENT";
	}

	return "Unknown";
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
	printf("%s event received\n", GetEventMessage(dwCtrlType));
	g_exit = true;
	return TRUE;
}

WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo *result = NULL;
struct addrinfo *ptr = NULL;
struct addrinfo	hints;
int iResult;
int recvbuflen = DEFAULT_BUFLEN;
HANDLE handle_heartbeat;
FILE *datanodefile;

int shutdownSocket();
void keepHeartBeat(void*);
int prepareSocket(int argc, char ** argv);
int loadDN();

int main(int argc, char** argv) {
	iResult = prepareSocket(argc, argv);
	if (iResult == 1) {
		return 1;
	}

	// handle_heartbeat = (HANDLE)_beginthread(keepHeartBeat, 0, NULL);

	loadDN();

	char inputbuf[1024];
	do {
		printf(">");
		gets_s(inputbuf, 1023);
		printf("\n[%s]", inputbuf);
		char* temp = NULL;
		char* temp_next = NULL;
		temp = strtok_s(inputbuf, " ", &temp_next);

		// switch只能分支基本类型，如果想判断char*：
		// 1. 用c++的constexpr和一个自己写的哈希函数，将字符串分支问题转化为int分支判断。
		// 2. if-else，因为是写cli，所以不用太追求效率，这样可以了
		if (strcmp(temp, "disconnect") == 0) {
			printf("disconnecting with %s", argv[1]);
			g_exit = true;
			break;
		}
		else if (strcmp(temp, "dnlist") == 0) {
		}
		else if (strcmp(temp, "dnrefresh") == 0) {
		}
		else if (strcmp(temp, "dnadd") == 0) {
		}
		else if (strcmp(temp, "dnremove") == 0) {
		}
		else if (strcmp(temp, "ls") == 0) {
		}
		else if (strcmp(temp, "pwd") == 0) {
		}
		else if (strcmp(temp, "cd") == 0) {
		}
		else if (strcmp(temp, "mkdir") == 0) {
		}
		else if (strcmp(temp,  "rm") == 0) {
		}
		else if (strcmp(temp,  "mv") == 0) {
		}
		else if (strcmp(temp,  "view") == 0) {
		}
		else if (strcmp(temp,  "pwd") == 0) {
		}
		else if (strcmp(temp, "help") == 0) {
			printf("\n'<dir>' parameter is must");
			printf("\n'[dir]' parameter is optional");
			printf("\nCOMMAND						USAGE");
			printf("\n-------------------------------------------------------");
			printf("\nheartbeat [ip]				heartbeat server");
			printf("\ndisconnect [ip]				disconnect with server");
			printf("\n");
			printf("\ndnlist						show datanode list");
			printf("\ndnrefresh [ip]				send heartbeat to datanode");
			printf("\ndnadd <ip>					add datanode");
			printf("\ndnremove <ip>					remove datanode");
			printf("\ndncon <ip>					try to connect a datanode");
			printf("\n");
			printf("\nls							list sub directory");
			printf("\npwd							print working directory");
			printf("\ncd <dir>						change directory");
			printf("\nmkdir <dir>					make directory");
			printf("\nrm <dir/file>					remove directory/file");
			printf("\nmv <dir/file> <targetdir>	mv	directory/file to directory");
			printf("\nview <file>					view first 100bytes and information of file");
			printf("\nupload <file>					upload file");
		}
		else if (strcmp(temp, "status")) {
			
		}
		else {
			printf("\nno such command: %s.");
			printf("\ninput 'help' for all commands avaliable.");
		}

		memset(inputbuf, 0, 1024);
	} while (true);

	// CloseHandle(handle_heartbeat);

	iResult = shutdownSocket();
	if (iResult == 1) {
		return 1;
	}

	return 0;
}

void keepHeartBeat(void *)
{
	ServerMessage servermsg;
	int type = 0;
	const char *text = "heartbeat\0";

	memcpy_s(&servermsg.type, sizeof(servermsg.type), &type, sizeof(type));
	memcpy_s(servermsg.msg,
		strlen(text) + 1,
		text,
		strlen(text) + 1);

	// 4. 发送一条消息
	int temp = sizeof(servermsg.type) + strlen(servermsg.msg);

	do {
		iResult = send(ConnectSocket, (char *)&servermsg, temp, 0);
		if (iResult == SOCKET_ERROR) {
			printf("Send failed with error: %d\n", WSAGetLastError());
			//closesocket(ConnectSocket);
			//WSACleanup();
		}
		Sleep(30*1000);
	} while (!g_exit);

	//printf("%ld bytes send.\n", iResult);
	//iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	//printf("message received: %s", recvbuf);
	//ZeroMemory(recvbuf, sizeof(recvbuf));
	//if (iResult > 0)
	//	printf("Bytes received: %d\n", iResult);
	//else if (iResult == 0)
	//	printf("Connection closed\n");
	//else
	//	printf("recv failed with error: %d\n", WSAGetLastError());
}

int shutdownSocket()
{
	// 5. 关闭连接
	// 单方向关闭连接，SD_SEND表示是SENDER提出ShutDown
	iResult = shutdown(ConnectSocket, SD_SEND);
	// WSASendDisconnect功能与shutdown相同，但可以多附加一条关闭信息
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	// 关闭整个socket并释放DLL
	closesocket(ConnectSocket);
	WSACleanup();

	printf("Client Closed.");
	return 0;
}

int prepareSocket(int argc, char ** argv)
{
	printf("Client Starting ...");

	// 保证命令行参数正常
	if (argc != 2) {
		printf("usage: %s server-name\n", argv[0]);
		return 1;
	}

	// 1. 初始化Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	// 关于hints（sockaddr）结构体的说明：
	// https ://docs.microsoft.com/zh-cn/windows/desktop/WinSock/sockaddr-2
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	// 获取并解析ip地址信息，argv1就是ip地址或者域名
	iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// 尝试连接每个socket都尝试，这个没看懂是为什么
	// 对ai_next的解释：调用getaddrinfo后会返回一个链表的addrinfo，每次调用ai_next就会获取一个，直到NULL
	// 网上的另种解释:
	//  （1） 如果nodename是字符串型的IPv6地址，bind的时候会分配临时端口；
	//	（2） 如果nodename是本机名，servname为NULL，则根据操作系统的不同略有不同
	//	注意点是： 这个函数，说起来，是get ，但是其实可以理解为creat 或者是理解为构建 。 因为你可以随意构建自己的地址结构addrinfo。
	//	如 果本函数返回成功，那么由result参数指向的变量已被填入一个指针，它指向的是由其中的ai_next成员串联起来的addrinfo结构链表。可以 导致返回多个addrinfo结构的情形有以下2个：
	//	1.    如果与hostname参数关联的地址有多个，那么适用于所请求地址簇的每个地址都返回一个对应的结构。
	//	2.    如果service参数指定的服务支持多个套接口类型，那么每个套接口类型都可能返回一个对应的结构，具体取决于hints结构的ai_socktype 成员。
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// 2. 创建一个socket
		// 尝试创建socket，端口未开启或占用会导致错误
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;

		}
		// 3. 将socket连接到服务器
		// 让socket连接到服务器，服务器拒绝连接后，client就换下一个socket尝试
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	// 创建socket对象完成，不再需要socket
	freeaddrinfo(result);

	// 连接失败
	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	return 0;
}

int loadParameters()
{
	fopen_s(&datanodefile, DNFILENAME, "r+");
	return 0;
}

int sendServerPackage(ServerMessage *msg) {
	// 这里用strlen是为了仅发送字符串，但是使用前一定要注意char[]最后的\0，例如：
	//memcpy_s(servermsg.msg,
	//	strlen(text) + 1,
	//	text,
	//	strlen(text) + 1);

	int temp = sizeof(msg->type) + strlen(msg->msg);
	int r = send(ConnectSocket, (char *)&msg, temp, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());

	}
}