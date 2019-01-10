// 自动生成
#include <windows.h>
#include <iostream>
#include <process.h>    /* _beginthread, _endthread */
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <ctime>
#include "sqlite3.h"
#include<string.h>

void threadAFunc(void *);
void threadBFunc(void *);
void getCurrentTime();
void test(int &a, int &b);

typedef struct Test
{
	int type;
	Test * next;
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
	/*
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
	//sqlite3 *db;
	//char *zErrMsg = 0;
	//int  rc;
	//char *sql;

	//rc = sqlite3_open("test.db", &db);

	//if (rc) {
	//	fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	//	exit(0);
	//} else {
	//	fprintf(stderr, "Opened database successfully\n");
	//}

	//Create SQL statement
	//sql = (char *)"CREATE TABLE COMPANY(\
	//	ID INT PRIMARY KEY     NOT NULL,\
	//	NAME           TEXT    NOT NULL,\
	//	AGE            INT     NOT NULL,\
	//	ADDRESS        CHAR(50),\
	//	SALARY         REAL );";

	//Execute SQL statement
	//rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
	//if (rc != SQLITE_OK) {
	//	fprintf(stderr, "SQL error: %d\n", rc);
	//	sqlite3_free(zErrMsg);
	//}
	//else {
	//	fprintf(stdout, "Table created successfully\n");
	//}
	//sqlite3_close(db);

	//char inputbuf[1024];
	//do {
	//	printf(">");
	//	gets_s(inputbuf, 1023);
	//	printf("\n[%s]", inputbuf);
	//	char* temp = NULL;
	//	char* temp_next = NULL;
	//	temp = strtok_s(inputbuf, " ", &temp_next);

	//	printf("\n[");

	//	while (temp != NULL) {
	//		printf("%s, ", temp);
	//		temp = strtok_s(NULL, " ", &temp_next);
	//	}
	//	printf("]\n");
	//	memset(inputbuf, 0, 1024);
	//} while (true);

	//FILE *datanodefile;
	//char line[17];
	//fopen_s(&datanodefile, "test", "r+");

	//if (datanodefile == NULL) {
	//	return 1;
	//}

	//while (!feof(datanodefile)) {
	//	if (fgets(line, 17, datanodefile) == NULL)
	//		return 2;
	//	//memset(line+16,'\0',1);
	//	printf("%s", line);
	//}

	//fclose(datanodefile);

	//int *a = (int*)malloc(sizeof(int));
	//int *b = a;
	//*a = 1;
	//printf("%d\n", *a);
	//printf("%d\n", *b);
	//printf("%p\n", a);
	//printf("%p\n", b);
	//if (a == b) {
	//	printf("233");
	//}
	//free(b);
	//// free的作用和java的gc.free类似。只不过java中这一过程是自动的，c是手动的。
	//// 这一步之后，c会在合适时间回收这个指针。
	//// 但需要注意的是，这个指针虽然free了，但它的值可能还会保留一段时间，所以它的值一定要及时设为NULL
	//// 防止以后再次使用这个指针，导致它指向一些不可知的值。
	////free(b);
	//b = NULL;
	////printf("%d\n", *a);
	////printf("%d\n", *b);
	//printf("%p\n", a);
	//printf("%p\n", b);

	//char test[6] = "aaaaa";
	//int a = strlen(test);
	//strcpy_s(test, "");
	////strcpy_s(test, "bbbbbb");
	//strcat_s(test, 3, "ccccc");
	//// 总之，strcat永远正确的写法：
	////strcat_s(ret, strlen(str1) + 1, str1);

	//Test *t = NULL;
	//Test **tt = &t;
	//Test *next = t->next;
	//printf("%p\n", tt);
	//t = (Test*)realloc(t, sizeof(Test));
	//t = (Test*)realloc(t, sizeof(Test));
	//t = (Test*)realloc(t, sizeof(Test));
	//free(t);
	//t = NULL;
	//printf("%p\n", &t);
	//printf("%p\n", tt);
*/
	
	/*

	char inputbuf[1024];

	while (true) {
		printf("\n");
		printf("> ");
		char* temp = NULL;
		char* temp_next = NULL;
		gets_s(inputbuf, 1023);
		temp = strtok_s(inputbuf, " ", &temp_next);
		if (temp != NULL && strcmp(temp, "p1")==0) {
			printf("\n");
			printf("parameter 1: %s", temp);
			printf("\n");
			printf("parameter 1: %s", temp_next);
		}
		temp = strtok_s(temp_next, " ", &temp_next);
		if (temp != NULL && strcmp(temp, "p2")==0) {
			printf("\n");
			printf("parameter 2: %s", temp);
			printf("\n");
			printf("parameter 2: %s", temp_next);
		}
		temp = strtok_s(temp_next, " ", &temp_next);
		if (temp != NULL && strcmp(temp, "p3")==0) {
			printf("\n");
			printf("parameter 3: %s", temp);
			printf("\n");
			printf("parameter 3: %s", temp_next);
		}
		memset(inputbuf, 0, 1024);
	}
	*/
	

	FILE *paramsfp = NULL;
	errno_t err;
	err = fopen_s(&paramsfp, "fuckyou", "r");
	if (err != 0) {
		// 为了首次使用能自动创建配置文件，这里用w+模式打开
		fopen_s(&paramsfp, "fuckyou", "w");
		fclose(paramsfp);
		// 并返回正常
		return 0;
	}
	// 如果未发生err而且fp还是NULL，那就是异常了
	if (paramsfp == NULL) {
		fclose(paramsfp);
		paramsfp = NULL;
		fopen_s(&paramsfp, "fuckyou", "w");
		return 1;
	}

	return 0;
}

void test(int &a, int &b) {
	printf("%d, %d", b, a);
	a = 9;
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
