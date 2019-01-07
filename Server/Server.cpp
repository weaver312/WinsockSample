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
WSADATA wsaData;
BOOL g_exit = FALSE;
char CTIME[24];
int iResult = 0;

int initialize();
void getCurrentTime();
// 主函数不声明也能运行是C的一个古老的问题，某些编译器会报warning甚至error
int main(void);

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

// filetree相关函数

/*
对服务器来说，一个socket应用通常有以下环节：
1. 初始化Winsock
2. 创建物理socket（ip + port）
2. 根据物理socket创建一个socket对象
3. 绑定socket对象到物理socket
4. 透过socket对象来监听Client
5. 接受Client的连接，建立一个线程为Client服务
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

	// 启动监听Client的线程
	_beginthread(acceptClientIncoming, 0, (void *)ClientListenSocket);

	int i = 0;

	do { } while (!g_exit);

	getCurrentTime();
	printf("%s Server closing...\n", CTIME);
	printf("------------------------------------------\n");
	// _beginthread内部会自动调用_endthread.
	//_endthread内部会自动调用CloseHandle关闭当前Thread内核对象的句柄
	//所以在用_beginthread 时我们不需要在主线程中调用CloseHandle来关闭子线程的句柄。
	//最多只需要调用_endthread，但也仅仅是为了"确保"资源的释放
	//CloseHandle(handle_incoming);
	//MSDN原文：
	//You can call _endthread or _endthreadex explicitly to terminate a thread;
	//however, _endthread or _endthreadex is called automatically when the thread
	//returns from the routine that's passed as a parameter. Terminating a thread
	//with a call to _endthread or _endthreadex helps ensure correct recovery of
	//resources that are allocated for the thread.

	shutdownClientSocket();

	getCurrentTime();
	printf("%s Ready to save datanode IPs...", CTIME);
	iResult = saveparam();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading datanodes!", CTIME);
		return 1;
	}
	
	getCurrentTime();
	printf("%s Ready to save filetree to local...", CTIME);
	iResult = saveFileTree();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading filetree!", CTIME);
		return 1;
	}
	
	
	getCurrentTime();
	printf("%s Cleanup-ing winsock lib...", CTIME);
	WSACleanup();
	printf("------------------------------------------\n");
	getCurrentTime();
	printf("%s Server closed, press any key to exit.\n", CTIME);
	getchar();

	return 0;
}

// 初始化：
// 1. 键监听
// 2. Client服务线程数组和Socket的存储数组
// 3. 初始化全局
// 4. 键监听
int initialize()
{
	// 公有初始化步骤
	if (SetConsoleCtrlHandler(HandlerRoutine, TRUE)) {
	} else {
		getCurrentTime();
		printf("%s error setting handler\n", CTIME);
		return 1;
	}
	// 1. 初始化Socket，无论是C还是S都要有这一步，否则会出10083错误代码
	clientiResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (clientiResult != 0) {
		getCurrentTime();
		printf("%s WSAStartup failed with error: %d\n", CTIME, clientiResult);
		return 1;
	}

	// 数组的初始化，这种写法可能有兼容性问题，有的编译器不一定支持这种写法
	// SOCKET ClientSocket[MAX_CLIENT];
	// ClientSocket[MAX_CLIENT] = { INVALID_SOCKET };
	memset(ClientThread, NULL, MAX_CLIENT * sizeof(HANDLE));
	memset(ClientSocket, INVALID_SOCKET, MAX_CLIENT * sizeof(SOCKET));
	memset(ClientSocketStatus, 0, MAX_CLIENT * sizeof(int));

	// 初始化头结点
	datanodes = (DataNode *)malloc(sizeof(DataNode));
	datanodes->status = -1;
	strcpy(datanodes->ip, "");
	datanodes->next = NULL;
	
	iResult = prepareClientSocket();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s PrepareClientSocket error!", CTIME);
		return 1;
	}

	// 载入datanodes列表
	getCurrentTime();
	printf("%s Ready to load datanode IPs...", CTIME);
	iResult = loadparams();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading datanodes!", CTIME);
		return 1;
	}

	getCurrentTime();
	printf("%s Ready to load filetree from json...", CTIME);
	iResult = loadFileTree();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading filetree!", CTIME);
		return 1;
	}

	return 0;
}

// Client相关变量
int	   clientiResult;
int    freeclientportnum = MAX_CLIENT;
int	   ClientSocketStatus[MAX_CLIENT];
HANDLE ClientThread[MAX_CLIENT];
SOCKET ClientSocket[MAX_CLIENT];
SOCKET ClientListenSocket = INVALID_SOCKET;
struct addrinfo *clientresult = NULL;
struct addrinfo clienthints;

// Client消息结构体
typedef struct CMDmsg
{
	int type;
	char msg[1020];
} CMDmsg;

// 将Client加入服务线程列表
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

		jsoncurnode[i] = jsonfiletree_root;
	}
}

// 处理Client发送来的包
void processClient(void *index)
{
	int myindex = (int)index;
	char recvbuf[CLIENT_BUFLEN];
	int recvbuflen = CLIENT_BUFLEN;
	CMDmsg recvpkg;
	bool connected = true;
	ZeroMemory(recvbuf, sizeof(recvbuf));

	while (!g_exit && connected) {
		if (ClientSocketStatus[myindex] == 0)
			continue;

		int iResult;
		iResult = recv(ClientSocket[myindex], recvbuf, recvbuflen, 0);

		// 结构体各域不一定连续，要分开赋值，且用secure函数
		char *currentAddr = recvbuf;
		memcpy_s(&recvpkg.type, sizeof(recvpkg.type), currentAddr, sizeof(recvpkg.type));
		currentAddr += sizeof(recvpkg.type);
		memcpy_s(recvpkg.msg, sizeof(recvpkg.msg), currentAddr, sizeof(recvpkg.msg));

		// If no byte received, current thread will be blocked in recv func.
		// If no error occurs, recv returns the number of bytes received and the buffer pointed to by the buf parameter will contain this data received.
		if (iResult > 0) {
			//printf("------------------------------\n");
			getCurrentTime();
			printf("%s %d Client Message received: type%d - %s\n", CTIME, myindex, recvpkg.type, recvpkg.msg);

			int reslen = 1020;
			DataNode *cur = datanodes->next;
			CMDmsg *sendpkg = (CMDmsg *)malloc(sizeof(CMDmsg));

			switch (recvpkg.type) {

			// heartbeat response
			case 100: {
				sendpkg->type = 100;
				strcpy_s(sendpkg->msg, "pit-a-pat");
				sendClientPackage(sendpkg, myindex);
				free(sendpkg);
				break;
			}

			// connect, 不存在, 连的是
			case 101: {
				break;
			}

			// disconnect
			case 102: {
				connected = false;

				sendpkg->type = 101;
				strcpy(sendpkg->msg, "disconnected with server.");
				sendClientPackage(sendpkg, myindex);
				free(sendpkg);
				break;
			}

			// list all datanodes
			case 110: {
				sendpkg->type = 110;
				strcpy(sendpkg->msg, "");

				while (cur != NULL && reslen > 24) {
					reslen -= 24;
					strcat_s(sendpkg->msg, 1020, cur->ip);
					strcat_s(sendpkg->msg, 1020, ";");
					cur = cur->next;
				}

				sendClientPackage(sendpkg, myindex);
			
				break;
			}
			
			// heartbeat all connected datanode
			case 111: {
				while (cur != NULL) {
					if (cur->status == 1) {
						sendpkg->type = 111;
						strcpy(sendpkg->msg, "heartbeat");
						sendDNPackage(cur->dnsocket, sendpkg);
					}
				}
				break;
			}

			// add a datanode
			case 112: {
				// 检查合法性

				// 添加到表里
				if (addparam(recvpkg.msg, 0) == 0) {
					sendpkg->type = 112;
					strcpy(sendpkg->msg, "add datanode success.");
					sendClientPackage(sendpkg, myindex);
				}
				else {
					sendpkg->type = 112;
					strcpy(sendpkg->msg, "add datanode fail, or duplicate exists probably");
				}
				break;
			}
			
			// remove a datanode
			case 113: {
				// 检查合法性

				// 从表中移除
				if (deleteparam(recvpkg.msg) == 0) {
					sendpkg->type = 113;
					strcpy(sendpkg->msg, "delete datanode success.");
					sendClientPackage(sendpkg, myindex);
				}
				else {
					sendpkg->type = 113;
					strcpy(sendpkg->msg, "delete datanode fail.");
				}
				break;
			}

			// try to connect a datanode
			case 114: {
				DataNode *returndn = NULL;

				// 尝试获取dn的ip
				if (fetchparam(recvpkg.msg, returndn) == 0) {
					sendpkg->type = 114;
					strcpy(sendpkg->msg, "establish connection request");

					sendDNPackage(returndn->dnsocket, sendpkg);
				}
				else {
					sendpkg->type = 114;
					strcpy(sendpkg->msg, "datanode not found on server, please add datanode first.");
				}
				break;
			}

			// try to connect ALL datanodes
			case 115: {
				connectEachDataNodes();
				break;
			}

			// ~ list sub directory
			case 121: {
				cJSON *cur = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "foldername");

				if (cur != NULL && cJSON_IsString(cur) && cur->string != NULL) {

					cJSON *folders = cJSON_GetObjectItemCaseSensitive(cur, "folders");
					cJSON *files   = cJSON_GetObjectItemCaseSensitive(cur, "files");
					cJSON *folder = NULL;
					int reslen = 1020;

					cJSON_ArrayForEach(folder, folders) {
						cJSON *foldername = cJSON_GetObjectItemCaseSensitive(folder, "foldername");
						if (cur != NULL && reslen > 24) {
							reslen -= 24;
							cur = cur->next;
							strcat_s(sendpkg->msg, 1020, foldername->string);
							strcat_s(sendpkg->msg, 1020, ";");
						}
					}
					cur->string;
				}

				sendpkg->type = 110;
				strcpy(sendpkg->msg, "");

				break;
			}

			// ~ print working directory
			case 122: {
				break;
			}

			// ~ change directory
			case 123: {
				cJSON *folders = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "folders");
				cJSON *folder = NULL;
				int founded = 0;

				cJSON_ArrayForEach(folder, folders) {
					if (strcmp(cJSON_GetObjectItemCaseSensitive(folder, "foldername")->string, recvpkg.msg) == 0) {
						jsoncurnode[myindex] = folder;
						founded = 1;
						break;
					}
				}

				sendpkg->type = 123;
				strcpy(sendpkg->msg, "DIR change ");
				if (founded == 0) {
					strcat(sendpkg->msg, "fail");
				}
				else {
					strcat(sendpkg->msg, "success");
				}
				sendClientPackage(sendpkg, myindex);

				break;
			}

			// ~ makedir
			case 124: {
				cJSON *folders = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "folders");
				cJSON *folder = NULL;
				int founded = 0;

				cJSON_ArrayForEach(folder, folders) {
					if (strcmp(cJSON_GetObjectItemCaseSensitive(folder, "foldername")->string, recvpkg.msg) == 0) {
						founded = 1;
						break;
					}
				}

				sendpkg->type = 124;
				if (founded == 0) {
					cJSON *newdir = cJSON_CreateObject();
					cJSON_AddStringToObject(newdir, "foldername", recvpkg.msg);
					cJSON_AddArrayToObject(newdir, "folders");
					cJSON_AddArrayToObject(newdir, "files");
					cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "folders"), newdir);

					strcpy(sendpkg->msg, "makedir success.");
				}
				else {
					strcpy(sendpkg->msg, "makedir failed, duplicate foldername existed.");
				}
				sendClientPackage(sendpkg, myindex);
			}

			// ~ remove directory/file
			case 125: {
				cJSON *folders = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "folders");
				cJSON *folder = NULL;
				int founded = 0;

				int folderindex = 0;
				cJSON_ArrayForEach(folder, folders) {
					if (strcmp(cJSON_GetObjectItemCaseSensitive(folder, "foldername")->string, recvpkg.msg) == 0) {
						founded = 1;
						cJSON_DeleteItemFromArray(folders, folderindex);
						break;
					}
					else {
						folderindex++;
					}
				}
				sendpkg->type = 125;
				if (founded == 1) {
					strcpy(sendpkg->msg, "dir remove success.");
				}
				else {
					strcpy(sendpkg->msg, "dir remove fail.");
				}
				sendClientPackage(sendpkg, myindex);
				break;
			}

			// ~ move file to directory/file
			case 126: {
				break;
			}

			// ~ view first 100bytes of file
			case 127: {
				break;
			}

			// ~ upload file
			case 128: {
				cJSON *files = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "files");
				cJSON *file = NULL;
				int founded = 0;

				int fileindex = 0;
				cJSON_ArrayForEach(file, files) {
					if (strcmp(cJSON_GetObjectItemCaseSensitive(file, "filename")->string, recvpkg.msg) == 0) {
						founded = 1;
						cJSON_DeleteItemFromArray(file, fileindex);
						break;
					}
					else {
						fileindex++;
					}
				}
				sendpkg->type = 125;
				if (founded == 1) {
					strcpy(sendpkg->msg, "file add to cjson success.");
				}
				else {
					strcpy(sendpkg->msg, "file add to cjson failed.");
				}
				sendClientPackage(sendpkg, myindex);
				break;
			}

			}
			// clear-up
			free(sendpkg);
			sendpkg = NULL;
			ZeroMemory(recvbuf, sizeof(recvbuf));

			// 回复发送方
			//printf("------------------------------\n");
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
			connected = false;
		}
		
		// Otherwise, a value of SOCKET_ERROR is returned, and a specific error code can be retrieved by calling WSAGetLastError.
		else {
			getCurrentTime();
			printf("%s Exception happened and %d client is closed. iResult is %d and WSAerror is %d.\n", CTIME, myindex, iResult, WSAGetLastError());
			connected = false;
		}
	}

	//关闭服务线程的顺序：
	//1.关闭本端作为发送方的端口
	iResult = shutdown(ClientSocket[myindex], SD_SEND);
	if (iResult == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s Client %d: try to stop sender with WSAerror: %d\n", CTIME, myindex, WSAGetLastError());
	}
	printf("%s %d Client just shut down.\n", CTIME, myindex);
	//2.彻底注销socket
	closesocket(ClientSocket[myindex]);
	//3.重新标记端口可用
	ClientSocketStatus[myindex] = 0;
	freeclientportnum++;
}

// 开始监听Client
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

// 逐个关闭Client服务线程和服务Socket
void shutdownClientSocket()
{
	for (int k = 0; k < MAX_CLIENT; k++) {
		if (ClientSocketStatus[k] != 0) {
			ClientSocketStatus[k] = 0;
			//CloseHandle(ClientThread[k]);
			int iResult = shutdown(ClientSocket[k], SD_SEND);
			if (iResult == SOCKET_ERROR) {
				getCurrentTime();
				printf("%s shutdown failed with error: %d\n", CTIME, WSAGetLastError());
			}
			closesocket(ClientSocket[k]);

			jsoncurnode[k] = NULL;
		}
	}
}

// 存储dn的链表节点
typedef struct DataNode
{
	int status;
	DataNode *next;
	SOCKET dnsocket;
	char ip[17];
} DataNode;

// DATANODE相关变量
int DN_alive = 0;
DataNode *datanodes;

// 尝试连接链表每个ip
int connectEachDataNodes() {	
	DataNode *cur = datanodes->next;
	int result;
	while (cur != NULL) {
		if (cur->status == 0) {
			result = connectSingleDataNode(cur);
			if (result == 0) {
				cur->status = 1;
				DN_alive++;
				getCurrentTime();
				printf("%s DN connected, ip[%s]", CTIME, cur->ip);
			}
			else {
				getCurrentTime();
				printf("%s DN connect failed, ip[%s]", CTIME, cur->ip);
			}
		}
		cur = cur->next;
	}
}

// 连接单个DataNode，把服务的Socket结构体赋给列表下标index
// 本函数只负责连接。心跳包的接收线程函数、断开连接等另有函数
// 传值需要引用传值
int connectSingleDataNode(DataNode * &curdn) {
	int dniResult;
	struct addrinfo dnhints;
	struct addrinfo *dnresult = NULL;
	struct addrinfo *dnptr = NULL;

	ZeroMemory(&dnhints, sizeof(dnhints));
	dnhints.ai_family = AF_UNSPEC;
	dnhints.ai_socktype = SOCK_STREAM;
	// 这里可以选用UDP等其他底层协议
	dnhints.ai_protocol = IPPROTO_TCP;

	dniResult = getaddrinfo(curdn->ip, PORT_DATANODE, &dnhints, &dnresult);
	if (dniResult != 0) {
		printf("getaddrinfo failed with error: %d\n", dniResult);
		return 1;
	}

	for (dnptr = dnresult; dnptr != NULL; dnptr = dnptr->ai_next) {
		curdn->dnsocket = socket(dnptr->ai_family, dnptr->ai_socktype, dnptr->ai_protocol);
		if (curdn->dnsocket == INVALID_SOCKET) {
			printf("socket create failed with error: %ld\n", WSAGetLastError());
			return 1;
		}
		dniResult = connect(curdn->dnsocket, dnptr->ai_addr, (int)dnptr->ai_addrlen);
		if (dniResult == SOCKET_ERROR) {
			closesocket(curdn->dnsocket);
			curdn->dnsocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(dnresult);
	freeaddrinfo(dnptr);

	// 确认连接成功/失败
	if (curdn->dnsocket == INVALID_SOCKET) {
		printf("Unable to connect datanode on ip[%s] !\n", curdn->ip);
		return 1;
	}
	return 0;
}

// 逐个断开列表中连接的Socket
int disconnOnlineDataNodes() {
	DataNode *cur = datanodes->next;
	int result;
	while (cur != NULL) {
		if (cur->status == 1) {
			result = disconnectSingleDataNode(cur);
			// 断开成功
			if (result == 0) {
				DN_alive--;
				getCurrentTime();
				printf("%s DN connected, ip[%s]", CTIME, cur->ip);
			}
			else {
				getCurrentTime();
				printf("%s DN connect failed, ip[%s]", CTIME, cur->ip);
			}
		}
		cur = cur->next;
	}
}

// 关闭单个DataNode的连接
int disconnectSingleDataNode(DataNode * &curdn) {
	int result;

	result = shutdown(curdn->dnsocket, SD_SEND);
	if (result == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(curdn->dnsocket);
		return 1;
	}

	closesocket(curdn->dnsocket);
	return 0;
}

// 载入配置文件的ip，如果没有ip则为空链表，仅含有已分配的datanodes表头（initialize函数中产生）
int loadparams()
{
	getCurrentTime();
	printf("%s Loading parameters\n", CTIME);

	// 保存头指针地址，最后要赋值回来。在此之前头结点已经被初始化
	DataNode ** begin= &datanodes;
	// 创建新节点的临时结构体
	DataNode *temp = NULL;

	FILE *paramsfp = NULL;
	errno_t err;
	err = fopen_s(&paramsfp, PARAMFILE, "r");
	if (err != 0) {
		// 为了首次使用能自动创建配置文件，这里用w+模式打开
		fopen_s(&paramsfp, PARAMFILE, "w+");
		fclose(paramsfp);
		// 并返回正常
		return 0;
	}
	// 如果未发生err而且fp还是NULL，那就是异常了
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
			//创建新节点
			temp = (DataNode *)malloc(sizeof(DataNode));
			strcpy_s(temp->ip, 17, ipline);
			temp->status = 0;
			temp->next = NULL;

			// 把新节点接到头的后面，并开始处理下一个节点
			datanodes->next = temp;
			datanodes = datanodes->next;
		}
		// 复原头结点地址，这步之后datanodes又是头结点了。头结点的status值是-1，其next才是第一个数据。
		datanodes = *begin;
	}
	printf("\n");
	fclose(paramsfp);

	getCurrentTime();
	printf("%s Loading parameters finished\n", CTIME);
	return 0;
}

// 添加dnip，返回0成功，1重复会失败
int addparam(char ip[], int status) {
	// check if exists duplicate ip param
	DataNode *temp;
	temp = datanodes->next;
	while (temp != NULL) {
		if (strcmp(datanodes->ip, ip) == 0) {
			return 1;
		}
		temp = datanodes->next;
	}
	free(temp);
	temp = NULL;

	temp = (DataNode *)malloc(sizeof(DataNode));
	strcpy_s(temp->ip, 17, ip);
	temp->status = status;
	temp->next = NULL;

	// insert after head 头插法
	temp->next = datanodes->next;
	datanodes->next = temp;
	return 0;
}

// 从链表中删除dnip
int deleteparam(char ip[]) {
	DataNode *pre = datanodes;
	DataNode *cur = datanodes->next;
	while (cur != NULL) {
		if (strcmp(cur->ip, ip) == 0) {
			// 如果是连接的dn，要断开连接
			if (cur->status == 1) {
				disconnectSingleDataNode(cur);
			}
			pre->next = cur->next;
			free(cur);
			cur = NULL;
			return 0;
		}
		else {
			pre = cur;
			cur = cur->next;
		}
	}
	// 未找到返回1
	return 1;
}

// dnip链表清空，非销毁
void clearparam() {
	DataNode *cur = datanodes->next;
	DataNode *temp = NULL;
	while (cur != NULL) {
		temp = cur->next;
		free(temp);
		cur = temp;
	}
	free(datanodes->next);
	datanodes->next = NULL;
}

// 保存dnip到本地，覆盖原先的配置文件
int saveparam() {
	FILE *paramsfp = NULL;
	errno_t err;
	err = fopen_s(&paramsfp, PARAMFILE, "w+");
	if (err != 0) return 1;
	if (paramsfp == NULL) {
		fclose(paramsfp);
		return 1;
	}

	DataNode *temp = datanodes;
	const char *nextline = "\n";
	while (temp != NULL && strlen(temp->ip) != 0) {
		fputs(temp->ip, paramsfp);
		fputs(nextline, paramsfp);
		temp = temp->next;
	}
	free(temp);
	temp = NULL;
	fclose(paramsfp);
	paramsfp = NULL;
	return 0;
}

// 从链表里尝试获取ip，0获取成功1不存在
int fetchparam(char ip[], DataNode *&returndn) {
	DataNode *temp = datanodes->next;
	while (temp != NULL) {
		if (strcmp(ip, temp->ip) == 0) {
			returndn = temp;
			return 0;
		}
		temp = temp->next;
	}
	return 1;
}

// 获取当前日期和时刻，字符串放在CTIME变量中
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

// 发送Client通信包
int sendClientPackage(CMDmsg *msg, int index) {
	// 这里用strlen是为了仅发送字符串部分，去掉多余长度。传值前要注意char[]最后的\0，例如：
	//memcpy_s(servermsg.sendpkg,
	//	strlen(text) + 1,
	//	text,
	//	strlen(text) + 1);
	// strcpy()会复制结束符\0过去，可以用

	// char *sendbuf = (char *)malloc(1024 * sizeof(char));
	// memcpy_s(sendbuf, sizeof(sendbuf), &msg->type, sizeof(msg->type));
	// memcpy_s(sendbuf + sizeof(msg->type), sizeof(sendbuf) - sizeof(msg->type), msg->msg, strlen(msg->msg));

	int temp = sizeof(msg->type) + strlen(msg->msg);
	int r = send(ClientSocket[index], (char *)&msg, temp, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

// 发送DataNode通信包
int sendDNPackage(SOCKET &dnsocket, CMDmsg *msg) {
	int temp = sizeof(msg->type) + strlen(msg->msg);
	int r = send(dnsocket, (char *)&msg, temp, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

char *filetreebuf;
FILE * filetreefp;
cJSON *jsonfiletree_root;
cJSON *jsoncurnode[MAX_CLIENT];

// 载入文件树，文件名由宏指定
int loadFileTree() {
	errno_t err;
	err = fopen_s(&filetreefp, FILETREEFILE, "r");
	if (err != 0) {
		fopen_s(&filetreefp, FILETREEFILE, "w+");
		fclose(filetreefp);
		jsonfiletree_root = cJSON_CreateObject();
		return 0;
	}
	if (filetreefp == NULL) {
		fclose(filetreefp);
		filetreefp = NULL;
		fopen_s(&filetreefp, FILETREEFILE, "w+");
		return 1;
	}

	fseek(filetreefp, 0, SEEK_END);
	int size = ftell(filetreefp);
	// 根据树大小动态申请空间
	filetreebuf = (char *)malloc(size + 1);
	filetreebuf[size] = '\0';

	fseek(filetreefp, 0L, SEEK_SET);
	fgets(filetreebuf, size, filetreefp);
	fclose(filetreefp);
	printf("Size of json treebuf: %ld bytes.\n", size);

	printf("cJSON tree loading...\n");
	jsonfiletree_root = cJSON_Parse(filetreebuf);
	printf("cJSON tree load finished.\n");
	if (jsonfiletree_root == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL) {
			fprintf(stderr, "Error before: %s\n", error_ptr);
			return 1;
		}
	}

	return 0;
}

// 保存文件树文件，并覆盖之前的
int saveFileTree() {
	FILE *filetreefp = NULL;
	errno_t err;
	err = fopen_s(&filetreefp, FILETREEFILE, "w+");
	if (err != 0) return 1;
	if (filetreefp == NULL) {
		fclose(filetreefp);
		return 1;
	}

	char *result = cJSON_Print(jsonfiletree_root);
	fputs(result, filetreefp);
	free(result);
	result = NULL;
	fclose(filetreefp);

	return 0;
}