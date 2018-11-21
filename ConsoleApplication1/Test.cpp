// 自动生成
#include "pch.h"
#include <windows.h>
#include <iostream>
#include <process.h>    /* _beginthread, _endthread */
#include <stdlib.h>
#include <stdio.h>

void threadAFunc(void *);
void threadBFunc(void *);

int main(int argc, char** argv) {
	// 关于创建线程函数的解释：whttps://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=vs-2017
	// 第1个是线程对应的功能函数
	// 第2个是堆栈大小，Windows会自动延伸堆栈的大小，0会在初始分配与父线程一样大小的堆栈。
	// 所以0就好。
	// 第3个是传过去的参数，如果是void *为唯一参数则不需要传参。
	// 注意前面的void *不能是没有参数，空参数需要用一个void *占位。
	_beginthread(threadAFunc, 0, NULL);
	_beginthread(threadBFunc, 0, NULL);
	while (true) {

	}
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
