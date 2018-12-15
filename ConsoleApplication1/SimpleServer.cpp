// �Զ�����
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
		// 0��������ctrl+c������ͨ��GenerateConsoleCtrlEvent���������ź�
	case CTRL_C_EVENT:
		return "CTRL_C_EVENT";

		// 1������ctrl+break����ͨ�����������ź�
	case CTRL_BREAK_EVENT:
		return "CTRL_BREAK_EVENT";

		// 2������GUI�Ĺر�console��ť
	case CTRL_CLOSE_EVENT:
		return "CTRL_CLOSE_EVENT";

		// 5��WIndows�û�ע��
	case CTRL_LOGOFF_EVENT:
		return "CTRL_LOGOFF_EVENT";

		// 6���û��ر���Windowsϵͳ
	case CTRL_SHUTDOWN_EVENT:
		return "CTRL_SHUTDOWN_EVENT";
	}

	return "Unknown";
}

BOOL WINAPI HandlerRoutine(DWORD dwCtrlType) {
	// ��������������Ͳ��գ�
	// whttps://docs.microsoft.com/zh-cn/windows/console/handlerroutine
	printf("%s event received\n", GetEventMessage(dwCtrlType));
	// �򵥴���������������������رո����ͻ�����һ����
	if (dwCtrlType >= 0)
		g_exit = TRUE;
	// ���return FALSE������ϵͳ�ͻ�����һ��Handler��������
	// ���return TRUE���ͻ�ػ������Ϣ�����ټ�������
	return TRUE;
}

/*
�Է�������˵��һ��socketӦ��ͨ�������»��ڣ�
1. ��ʼ��Winsock
2. ��������socket��ip + port��
2. ��������socket����һ��socket����
3. ��socket��������socket
4. ����Client
5. ����Client������
6. ������Ϣ/������Ϣ
7. �Ͽ����ӣ��ͷ���Դ
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

	// ��������ĳ�ʼ��������д�������м��������⣬�еı�������һ��֧������д��
	// SOCKET ClientSocket[MAX_CLIENT];
	// ClientSocket[MAX_CLIENT] = { INVALID_SOCKET };
	memset(ClientThread, NULL, MAX_CLIENT * sizeof(HANDLE));
	memset(ClientSocket, INVALID_SOCKET, MAX_CLIENT * sizeof(SOCKET));
	memset(ClientSocketStatus, 0, MAX_CLIENT * sizeof(int));

	// 1. ��ʼ��Socket��������C����S��Ҫ����һ����������10083�������
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	// 2. ����һ��socket
	// ָ����ַ�ṹ����ʾҪѡ�����һ��Э�����͵Ȳ���
	// ����hint��sockaddr���ṹ���˵����
	// https ://docs.microsoft.com/zh-cn/windows/desktop/WinSock/sockaddr-2
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	// ���Ը��ݶ˿ڽ������������˿ڿɲ����á�ǰ���sockaddr��ַ�ṹ��û�д���ȵ�
	iResult = getaddrinfo(NULL, PORT_HEARTBEAT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	// ���������������һ������Socket
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// 3. ��һ������socket������socket�ϣ�result��������socket��һЩ����
	// �󶨼���socket������socket��
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	// ��ν������socket����ʵ���Ǵ���socket�����һ���н���Ϣ��������socket�������Ϊ˳����ɣ�
	// Ҳ�Ͳ���Ҫǰ��׼��������socket��Ϣ��
	// ������������ͷŵ����"�����"socket address info
	freeaddrinfo(result);

	// 4. ���������ӿڣ����˳�ʼ�����
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	printf("initialized iResult is %d.\n", iResult);

	// ��������Client������߳�
	HANDLE handle_incoming = (HANDLE)_beginthread(acceptClientIncoming, 0, (void *)ListenSocket);
	// HANDLE handle_process = (HANDLE)_beginthread(processClient, 0, NULL);

	// 6. ���ջ�����Ϣ
	do {
		// 5. ���Խ���Client���������󣬲���ListenSocket������������ճ�ΪClientSocket
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
		//	// �ظ����ͷ�
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
		//	// ������Ҫ������ʱsocket���Թر�
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
		// ��ôд���У�ǰ��˿ڵĽ��ջ᲻�ϵ��������for����ֹ����Ķ˿ڼ���������Ϣ
		// ����Ӧ�ø�ÿ��Client������һ��thread
		iResult = recv(ClientSocket[myindex], recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("------------------------------\n");
			printf("%d Client:", myindex);
			printf("Bytes received: %d\n", iResult);
			printf("Message received: %s\n", recvbuf);
			printf("------------------------------\n");
			ZeroMemory(recvbuf, sizeof(recvbuf));
			// �ظ����ͷ�
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
