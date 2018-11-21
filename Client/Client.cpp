// 自动生成
#include "pch.h"
#include <iostream>
// 确保使用winsock1.1的部分兼容性
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// 端口和缓冲区大小
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

// 必要的包
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdlib.h>
#include <stdio.h>

// 需要链接的本地库
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

bool g_exit = false;

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

/*

*/

int main(int argc, char** argv) {
	printf("Client Starting ...");

	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char *sendbuf = (char*)"this is a test";
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

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

	// 4. 发送一条消息
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	printf("发送字节: %ld\n", iResult);
	// 准备接受数据
	
	do {
		//iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		//printf("message received: %s", recvbuf);
		//ZeroMemory(recvbuf, sizeof(recvbuf));
		//if (iResult > 0)
		//	printf("Bytes received: %d\n", iResult);
		//else if (iResult == 0)
		//	printf("Connection closed\n");
		//else
		//	printf("recv failed with error: %d\n", WSAGetLastError());
	} while (!g_exit);


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

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
