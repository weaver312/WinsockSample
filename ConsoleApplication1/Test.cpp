// �Զ�����
#include "pch.h"
#include <windows.h>
#include <iostream>
#include <process.h>    /* _beginthread, _endthread */
#include <stdlib.h>
#include <stdio.h>

void threadAFunc(void *);
void threadBFunc(void *);

int main(int argc, char** argv) {
	// ���ڴ����̺߳����Ľ��ͣ�whttps://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=vs-2017
	// ��1�����̶߳�Ӧ�Ĺ��ܺ���
	// ��2���Ƕ�ջ��С��Windows���Զ������ջ�Ĵ�С��0���ڳ�ʼ�����븸�߳�һ����С�Ķ�ջ��
	// ����0�ͺá�
	// ��3���Ǵ���ȥ�Ĳ����������void *ΪΨһ��������Ҫ���Ρ�
	// ע��ǰ���void *������û�в������ղ�����Ҫ��һ��void *ռλ��
	_beginthread(threadAFunc, 0, NULL);
	_beginthread(threadBFunc, 0, NULL);
	while (true) {

	}
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
