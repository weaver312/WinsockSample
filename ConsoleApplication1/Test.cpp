// �Զ�����
#include <windows.h>
#include <iostream>
#include <process.h>    /* _beginthread, _endthread */
#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <ctime>
#include "sqlite3.h"
#include <string.h>

#include "include/mbedtls/config.h"
#include "include/mbedtls/platform.h"
#define mbedtls_printf     printf

#include "include/mbedtls/md5.h"
#include "include/mbedtls/sha256.h"

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
	// ���ڴ����̺߳����Ľ��ͣ�whttps://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=vs-2017
	// ��1�����̶߳�Ӧ�Ĺ��ܺ���
	// ��2���Ƕ�ջ��С��Windows���Զ������ջ�Ĵ�С��0���ڳ�ʼ�����븸�߳�һ����С�Ķ�ջ��
	// ����0�ͺá�
	// ��3���Ǵ���ȥ�Ĳ����������void *ΪΨһ��������Ҫ���Ρ�
	// ע��ǰ���void *������û�в������ղ�����Ҫ��һ��void *ռλ��
	// _beginthread(threadafunc, 0, null);
	// _beginthread(threadbfunc, 0, null);
	// while (true) {
	// }
	
	//time_t currenttime;
	//struct tm currentlocaltime;

	//time(&currenttime);
	//localtime_s(&currentlocaltime, &currenttime);
	//char result[24];
	//// i ��ʾ��ʽ���ַ������ȣ��������Ӽ���������û��
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
	//// free�����ú�java��gc.free���ơ�ֻ����java����һ�������Զ��ģ�c���ֶ��ġ�
	//// ��һ��֮��c���ں���ʱ��������ָ�롣
	//// ����Ҫע����ǣ����ָ����Ȼfree�ˣ�������ֵ���ܻ��ᱣ��һ��ʱ�䣬��������ֵһ��Ҫ��ʱ��ΪNULL
	//// ��ֹ�Ժ��ٴ�ʹ�����ָ�룬������ָ��һЩ����֪��ֵ��
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
	//// ��֮��strcat��Զ��ȷ��д����
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
	err = fopen_s(&paramsfp, "test", "r");
	if (err != 0) {
		fopen_s(&paramsfp, "test", "w");
		fclose(paramsfp);
		return 0;
	}
	
	int length = ftell(paramsfp);
	printf("%d\n", length);

	fseek(paramsfp, 0, SEEK_END);
	length = ftell(paramsfp);
	printf("%d\n", length);

	char buf[1024];
	fseek(paramsfp, 0, SEEK_SET);
	int readLen = fread(buf, 1, 1024, paramsfp);
	printf("%d\n", readLen);

	printf("------------------------\n");

	fseek(paramsfp, 0, SEEK_SET);
	for (int i = 0; i < 5; i++) {
		fseek(paramsfp, 10, SEEK_CUR);
		int length = ftell(paramsfp);
		printf("%d\n", length);
	}

	return 0;
}

void test(int &a, int &b) {
	printf("%d, %d", b, a);
	a = 9;
}

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
	// ���ڰ�ȫ�ԣ�Ҫ��ʹ��localtime_s��linux��ʹ��localtime_r
	localtime_s(&currentlocaltime, &currenttime);
	// i ��ʾ��ʽ���ַ������ȣ��������Ӽ���������û��
	int i = snprintf(CTIME, sizeof(CTIME), "%02d-%02d-%02d %02d:%02d:%02d",
		currentlocaltime.tm_year + 1900,
		currentlocaltime.tm_mon + 1,
		currentlocaltime.tm_mday,
		currentlocaltime.tm_hour,
		currentlocaltime.tm_min,
		currentlocaltime.tm_sec);
}
