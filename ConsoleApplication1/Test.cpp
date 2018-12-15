// 自动生成
#include <windows.h>
#include <iostream>
#include <process.h>    /* _beginthread, _endthread */
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <ctime>
#include "sqlite3.h"

void threadAFunc(void *);
void threadBFunc(void *);
void getCurrentTime();

typedef struct Test
{
	int type;
	char msg[128];
} Test;

char CTIME[24];

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;
	for (i = 0; i < argc; i++) {
		printf("%d: %s = %s\n",i, azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int main(int argc, char** argv) {
	// 关于创建线程函数的解释：whttps://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=vs-2017
	// 第1个是线程对应的功能函数
	// 第2个是堆栈大小，Windows会自动延伸堆栈的大小，0会在初始分配与父线程一样大小的堆栈。
	// 所以0就好。
	// 第3个是传过去的参数，如果是void *为唯一参数则不需要传参。
	// 注意前面的void *不能是没有参数，空参数需要用一个void *占位。
	// _beginthread(threadafunc, 0, null);
	// _beginthread(threadbfunc, 0, null);
	// while (true) {
	// }
	
	//time_t currenttime;
	//struct tm currentlocaltime;

	//time(&currenttime);
	//localtime_s(&currentlocaltime, &currenttime);
	//char result[24];
	//// i 表示格式化字符串长度，具体例子见网，这里没用
	//int i = snprintf(result, sizeof(result), "%02d-%02d-%02d %02d:%02d:%02d",
	//	currentlocaltime.tm_year + 1900,
	//	currentlocaltime.tm_mon + 1,
	//	currentlocaltime.tm_mday,
	//	currentlocaltime.tm_hour,
	//	currentlocaltime.tm_min,
	//	currentlocaltime.tm_sec);
	//printf("%s", result);

	//Test test;
	//printf("%d", sizeof(test));
	//getCurrentTime();
	//printf("%s", CTIME);
	//char * t = (char *)"fffffffffffffffff";
	//printf("%d", strlen(t));

	// test sqlite3
	sqlite3 *db;
	char *zErrMsg = 0;
	int  rc;
	char *sql;

	rc = sqlite3_open("test.db", &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	} else {
		fprintf(stderr, "Opened database successfully\n");
	}

	/* Create SQL statement */
	sql = (char *)"CREATE TABLE COMPANY(\
		ID INT PRIMARY KEY     NOT NULL,\
		NAME           TEXT    NOT NULL,\
		AGE            INT     NOT NULL,\
		ADDRESS        CHAR(50),\
		SALARY         REAL );";

	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %d\n", rc);
		sqlite3_free(zErrMsg);
	}
	else {
		fprintf(stdout, "Table created successfully\n");
	}
	sqlite3_close(db);

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

void threadAFunc(void *)
{
	while (true) {
		printf("Thread A.\n");
		Sleep(100);
	}
}

void threadBFunc(void *)
{
	while (true) {
		printf("Thread B.\n");
		Sleep(100);
	}
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
