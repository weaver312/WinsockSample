// �Զ�����
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

// ���г���: Ctrl + F5 ����� >����ʼִ��(������)���˵�
// ���Գ���: F5 ����� >����ʼ���ԡ��˵�

// ������ʾ: 
//   1. ʹ�ý��������Դ�������������/�����ļ�
//   2. ʹ���Ŷ���Դ�������������ӵ�Դ�������
//   3. ʹ��������ڲ鿴���������������Ϣ
//   4. ʹ�ô����б��ڲ鿴����
//   5. ת������Ŀ��>���������Դ����µĴ����ļ�����ת������Ŀ��>�����������Խ����д����ļ���ӵ���Ŀ
//   6. ��������Ҫ�ٴδ򿪴���Ŀ����ת�����ļ���>���򿪡�>����Ŀ����ѡ�� .sln �ļ�

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
