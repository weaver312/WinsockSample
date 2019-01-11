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
WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo *result = NULL;
struct addrinfo *ptr = NULL;
struct addrinfo	hints;
int iResult;
HANDLE handle_heartbeat;
FILE *datanodefile;

int main(int argc, char** argv) {

	iResult = prepareSocket(argc, argv);
	if (iResult == 1) {
		printf("prepare socket error\n");
		return 1;
	}
	
	// handle_heartbeat = (HANDLE)_beginthread(keepHeartBeat, 0, NULL);

	char inputbuf[1024];
	do {
		CMDmsg sendpkg;
		CMDmsg recvpkg;

		printf("\n> ");
		ZeroMemory(inputbuf, 1024);
		gets_s(inputbuf, 1023);
		char* temp = NULL;
		char* temp_next = NULL;
		temp = strtok_s(inputbuf, " ", &temp_next);

		// switch只能分支基本类型，如果想判断char*：

		if (temp == NULL) continue;

		if (strcmp(temp, "status") == 0) {
			printf("server ip: %s\n", argv[1]);
		}
		else if (strcmp(temp, "heartbeat") == 0) {
			sendpkg.type = 100;
			strcpy_s(sendpkg.msg, "Manual heartbeat");
			sendServerPackage(&sendpkg);
			recvServerPackage(&recvpkg);
		}
		else if (strcmp(temp, "disconnect") == 0) {
			sendpkg.type = 102;
			strcpy_s(sendpkg.msg, "Client abort");
			sendServerPackage(&sendpkg);
			printf("\n");
			printf("disconnecting with %s", argv[1]);
			printf("\n");
			g_exit = true;
			break;
		}
		else if (strcmp(temp, "dnlist") == 0) {
			sendpkg.type = 110;
			strcpy_s(sendpkg.msg, "Fetch datanode list");
			sendServerPackage(&sendpkg);
			recvServerPackage(&recvpkg);
			printf("\n");
			printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
			printf("\n");
		}
		else if (strcmp(temp, "dnrefresh") == 0) {
			sendpkg.type = 111;
			strcpy_s(sendpkg.msg, "Refresh all datanodes");
			sendServerPackage(&sendpkg);
			recvServerPackage(&recvpkg);
			printf("\n");
			printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
			printf("\n");
		}
		else if (strcmp(temp, "dnadd") == 0) {
			sendpkg.type = 112;
			temp = strtok_s(temp_next, " ", &temp_next);
			if (temp != NULL && strlen(temp) <= 15) {
				strcpy_s(sendpkg.msg, temp);
				sendServerPackage(&sendpkg);
				recvServerPackage(&recvpkg);
				printf("\n");
				printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
				printf("\n");
			}
			else {
				printf("\n");
				printf("USAGE: dnadd [ip]");
				printf("\n");
			}
		}
		else if (strcmp(temp, "dnremove") == 0) {
			sendpkg.type = 113;
			temp = strtok_s(temp_next, " ", &temp_next);
			if (temp != NULL && strlen(temp) <= 15) {
				strcpy_s(sendpkg.msg, temp);
				sendServerPackage(&sendpkg);
				recvServerPackage(&recvpkg);
				printf("\n");
				printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
				printf("\n");
			}
			else {
				printf("\n");
				printf("USAGE: dnremove [ip]");
				printf("\n");
			}
		}
		else if (strcmp(temp, "dncon") == 0) {
			temp = strtok_s(temp_next, " ", &temp_next);
			if (temp != NULL) {
				if (strlen(temp) <= 15) {
					sendpkg.type = 114;
					strcpy_s(sendpkg.msg, temp);
					sendServerPackage(&sendpkg);
					recvServerPackage(&recvpkg);
					printf("\n");
					printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
					printf("\n");
				}
				else {
					printf("\n");
					printf("illegal ip address: %s", temp);
					printf("\n");
				}
			}
			else {
				sendpkg.type = 115;
				strcpy_s(sendpkg.msg, "Connect all datanodes");
				sendServerPackage(&sendpkg);
				recvServerPackage(&recvpkg);
				printf("\n");
				printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
				printf("\n");
			}
		}
		else if (strcmp(temp, "ls") == 0) {
			sendpkg.type = 121;
			strcpy_s(sendpkg.msg, "List sub directory");
			sendServerPackage(&sendpkg);
			recvServerPackage(&recvpkg);
			printf("\n");
			printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
			printf("\n");
		}
		else if (strcmp(temp, "pwd") == 0) {
			sendpkg.type = 122;
			strcpy_s(sendpkg.msg, "Print working directory");
			sendServerPackage(&sendpkg);
			recvServerPackage(&recvpkg);
			printf("\n");
			printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
			printf("\n");
		}
		else if (strcmp(temp, "cd") == 0) {
			sendpkg.type = 123;
			temp = strtok_s(temp_next, " ", &temp_next);
			if (temp != NULL) {
				strcpy_s(sendpkg.msg, temp);
				sendServerPackage(&sendpkg);
				recvServerPackage(&recvpkg);
				printf("\n");
				printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
				printf("\n");
			}
			else {
				printf("\n");
				printf("USAGE: cd  [foldername]");
				printf("\n");
			}
		}
		else if (strcmp(temp, "mkdir") == 0) {
			sendpkg.type = 124;
			temp = strtok_s(temp_next, " ", &temp_next);
			if (temp != NULL) {
				strcpy_s(sendpkg.msg, temp);
				sendServerPackage(&sendpkg);
				recvServerPackage(&recvpkg);
				printf("\n");
				printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
				printf("\n");
			}
			else {
				printf("\n");
				printf("USAGE: mkdir [foldername]");
				printf("\n");
			}
		}
		else if (strcmp(temp, "rm") == 0) {
			sendpkg.type = 125;
			temp = strtok_s(temp_next, " ", &temp_next);
			if (temp != NULL) {
				strcpy_s(sendpkg.msg, temp);
				sendServerPackage(&sendpkg);
				recvServerPackage(&recvpkg);
				printf("\n");
				printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
				printf("\n");
			}
			else {
				printf("\n");
				printf("USAGE: rm [folder/filename]");
				printf("\n");
			}
		}
		else if (strcmp(temp, "mv") == 0) {
		}
		else if (strcmp(temp, "view") == 0) {
		}
		else if (strcmp(temp, "upload") == 0) {
			sendpkg.type = 128;
			temp = strtok_s(temp_next, " ", &temp_next);
			if (temp != NULL) {
				strcpy_s(sendpkg.msg, temp);
				sendServerPackage(&sendpkg);
				recvServerPackage(&recvpkg);
				printf("\n");
				printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
				printf("\n");
			} 
			else {
				printf("\n");
				printf("USAGE: upload [filename]");
				printf("\n");
			}
		}
		else if (strcmp(temp, "help") == 0) {
			printf("\n'<dir>' parameter is must");
			printf("\n'[dir]' parameter is optional");
			printf("\nCOMMAND            USAGE");
			printf("\n-----------------------------------------------");
			printf("\nstatus             show current status");
			printf("\nheartbeat [ip]     heartbeat server");
			printf("\ndisconnect [ip]    disconnect with server");
			printf("\n");
			printf("\ndnlist             show datanode list");
			printf("\ndnrefresh [ip]     send heartbeat to datanode");
			printf("\ndnadd <ip>         add datanode");
			printf("\ndnremove <ip>      remove datanode");
			printf("\ndnconall           connect ALL datanodes");
			printf("\n");
			printf("\nls                 list sub directory");
			printf("\npwd                print working directory");
			printf("\ncd <dir>           change directory");
			printf("\nmkdir <dir>        make directory");
			printf("\nrm <dir/file>      remove directory/file");
			printf("\nmv <from> <to>     directory/file to directory");
			printf("\nview <file>        view first 100bytes of file");
			printf("\nupload <file>      upload file");
		}
		else {
			printf("\nno such command: %s.", temp);
			printf("\ninput 'help' for all commands avaliable.");
			printf("\n");
		}
	} while (true);

	// CloseHandle(handle_heartbeat);

	iResult = shutdownSocket();
	if (iResult == 1) {
		printf("shutdown socket error\n");
		return 1;
	}

	return 0;
}

void keepHeartBeat(void *)
{
	CMDmsg servermsg;
	int type = 1;
	const char *text = "heartbeat package type 1\0";

	servermsg.type = type;
	strcpy_s(servermsg.msg, text);

	// 4. 发送一条消息
	int temp = sizeof(servermsg.type) + strlen(servermsg.msg);

	do {
		iResult = send(ConnectSocket, (char *)&servermsg, temp, 0);
		if (iResult == SOCKET_ERROR) {
			printf("Send failed with error: %d\n", WSAGetLastError());
		}
		Sleep(30*1000);
	} while (!g_exit);
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
	printf("Client Starting ...\n");

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

int sendServerPackage(CMDmsg *msg) {
	// 这里用strlen是为了仅发送字符串，但是使用前一定要注意char[]最后的\0，例如：
	//memcpy_s(servermsg.msg,
	//	strlen(text) + 1,
	//	text,
	//	strlen(text) + 1);

	char *sendbuf = (char *)malloc(1024 * sizeof(char));
	int temp = sizeof(msg->type) + strlen(msg->msg);
	memcpy_s(sendbuf, 1024 * sizeof(char), &msg->type, sizeof(msg->type));
	strcpy_s(sendbuf + sizeof(msg->type), 1024 * sizeof(char) - sizeof(msg->type), msg->msg);
	//memcpy_s(sendbuf + sizeof(msg->type), 1024 * sizeof(char) - sizeof(msg->type), msg->msg, strlen(msg->msg));

	int r = send(ConnectSocket, sendbuf, temp, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	free(sendbuf);
	return 0;
}

int recvServerPackage(CMDmsg *recvpkg) {
	char recvbuf[1024];
	ZeroMemory(recvbuf, sizeof(recvbuf));
	int r = recv(ConnectSocket, recvbuf, DEFAULT_BUFLEN, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	
	char *currentAddr = recvbuf;
	memcpy_s(&recvpkg->type, sizeof(recvpkg->type), currentAddr, sizeof(recvpkg->type));
	currentAddr += sizeof(recvpkg->type);
	// 收包用strcpy_s加\0了也会报错，不知道为啥
	memcpy_s(recvpkg->msg, sizeof(recvpkg->msg), currentAddr, sizeof(recvpkg->msg));

	return 0;
}

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