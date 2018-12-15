#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <ctime>

#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define CLIENT_BUFLEN 1024
#define MAX_CLIENT 8
#define PORT_CLIENT "27015"

// winsock相关库
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
// _beginthread, _endthread
#include <process.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

BOOL g_exit = FALSE;
char CTIME[24];

void initialize();
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

int main(int argc, char** argv) {
	initialize();
	getCurrentTime();
	printf("%s Server starting ...\n", CTIME);
}

void initialize()
{
	if (SetConsoleCtrlHandler(HandlerRoutine, TRUE)) {
	}
	else {
		getCurrentTime();
		printf("%s error setting handler\n", CTIME);
		exit(1);
	}

	// 对于数组的初始化，这种写法可能有兼容性问题，有的编译器不一定支持这种写法
	// SOCKET ClientSocket[MAX_CLIENT];
	// ClientSocket[MAX_CLIENT] = { INVALID_SOCKET };
	memset(ClientThread, NULL, MAX_CLIENT * sizeof(HANDLE));
	memset(ClientSocket, INVALID_SOCKET, MAX_CLIENT * sizeof(SOCKET));
	memset(ClientSocketStatus, 0, MAX_CLIENT * sizeof(int));
}

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
