#include "pch.h"
#include "Client.h"

bool g_exit = false;
WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;
struct addrinfo *result = NULL;
struct addrinfo *ptr = NULL;
struct addrinfo	hints;
int	iResult;
HANDLE handle_heartbeat;
FILE *datanodefile;
int heartbeatduration = 30;

int main(int argc, char** argv) {

	iResult = prepareSocket(argc, argv);
	if (iResult == 1) {
		printf("prepare socket error\n");
		return 1;
	}
	// handle_heartbeat = (HANDLE)_beginthread(keepHeartBeat, 0, (void*)heartbeatduration);

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

		if (temp == NULL) continue;

		if (strcmp(temp, "status") == 0) {
			printf("server ip: %s\n", argv[1]);
		}
		else if (strcmp(temp, "heartbeat") == 0) {
			sendpkg.type = 100;
			strcpy_s(sendpkg.msg, "Manual heartbeat");
			sendServerPackage(&sendpkg);
			recvServerPackage(&recvpkg);
			printf("\n");
			printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
			printf("\n");
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
			// 获取要上传的文件名
			char * filename = strtok_s(temp_next, " ", &temp_next);
			if (filename != NULL && strcmp(filename, "")) {
				// 尝试打开
				FILE *uploadfile = NULL;
				errno_t err;
				err = fopen_s(&uploadfile, filename, "rb");
				if (uploadfile == NULL || err != 0) {
					printf("\n");
					printf("file not exist!");
					printf("\n");
				}
				else {
					fseek(uploadfile, 0, SEEK_SET);
					fseek(uploadfile, 0, SEEK_END);
					long filebytenum = ftell(uploadfile);
					int devideblocknums = (filebytenum / (long)(1024 * 1024 * 64)) + 1;
					fseek(uploadfile, 0, SEEK_SET);

					// 第1步，获取文件名和全路径，计算hash，把128包传给server
					sendpkg.type = 122;
					strcpy_s(sendpkg.msg, "Print working directory");
					sendServerPackage(&sendpkg);
					ZeroMemory(sendpkg.msg, 1020);
					recvServerPackage(&recvpkg);
					printf("\n");
					printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
					printf("\n");
					char filepath[1024];
					ZeroMemory(filepath, 1024);
					strcpy_s(filepath, recvpkg.msg);
					// 给路径名接上文件名
					strcat_s(filepath, filename);

					unsigned char filedigest[33];
					ZeroMemory(filedigest, sizeof(filedigest));
					mbedtls_sha256((unsigned char *)filepath, strlen(filepath), filedigest, 0);

					printf("file SHA256 digest:\n");
					for (int i = 0; i < 32; i++) {
						printf("%02x", filedigest[i]);
					}
					printf("\n");
					char filedigeststr[64 + 1];
					ZeroMemory(filedigeststr, 0);
					for (int i = 0; i < 32; i++) {
						char singlebuffer[2 + 1];
						sprintf_s(singlebuffer, "%02x", filedigest[i]);
						memcpy_s(filedigeststr + i * 2, 64, singlebuffer, 2);
					}
					filedigeststr[64] = '\0';

					for (int whichblocknum = 1; whichblocknum <= devideblocknums; whichblocknum++)
					{
						int thisblocklength = (whichblocknum==devideblocknums) ? filebytenum%(64 * 1024 * 1024) : (64 * 1024 * 1024);

						// SHA256的摘要长32字节
						char * fileblock = (char *)malloc(thisblocklength);
						fread(fileblock, 1, thisblocklength, uploadfile);

						// 计算blockhash
						unsigned char blockdigest[33];
						ZeroMemory(blockdigest, sizeof(blockdigest));
						mbedtls_sha256((unsigned char *)fileblock, thisblocklength, blockdigest, 0);
						printf("#%d/%d block SHA256 digest:\n", whichblocknum, devideblocknums);
						for (int i = 0; i < 32; i++) {
							printf("%02x", blockdigest[i]);
						}
						printf("\n");

						char blockdigeststr[64 + 1];
						ZeroMemory(blockdigeststr, 0);
						for (int i = 0; i < 32; i++) {
							char singlebuffer[2 + 1];
							sprintf_s(singlebuffer, "%02x", blockdigest[i]);
							memcpy_s(blockdigeststr + i * 2, 64, singlebuffer, 2);
						}
						blockdigeststr[64] = '\0';

						for (int i = 1; i <= BLOCKDUPNUM; i++)
						{
							// 嵌套的for，BLOCKDUPNUM次
							// 获取所有已连接(status == 1)的DataNodes，按顺序挨个循环发送Block
							sendpkg.type = 201;
							strcpy_s(sendpkg.msg, "Fetch alive datanode list.");
							sendServerPackage(&sendpkg);
							ZeroMemory(sendpkg.msg, 1020);
							recvServerPackage(&recvpkg);
							if (strlen(recvpkg.msg) == 0 || strcmp(recvpkg.msg, "") == 0) {
								printf("error: no alive datanode avaliable, stop sending block to DN.");
								printf("\n");
								break;
							}
							printf("Alive DN: %s", recvpkg.msg);
							printf("\n");

							// 第2步：
							// 让Server存信息
							// BLOCKHASH	TEXT	char * blockdigest
							// BLOCKNUM		INT		int	   whichblocknum
							// DUPLICATE	INT		int	   i
							// FILEHASH		TEXT	char * filedigest
							// DATANODEIP	TEXT	char * datanodeip
							char tempitoa[4];
							ZeroMemory(tempitoa, 4);

							SOCKET dnsocket;
							printf("\n");
							printf("connecting to DN %s.", recvpkg.msg);
							printf("\n");
							iResult = buildConntoDN(recvpkg.msg, dnsocket);
							if (iResult != 0) {
								printf("\n");
								printf("build conn to DN %s failed.", recvpkg.msg);
								printf("\n");
							}

							sendpkg.type = 129;
							strcat_s(sendpkg.msg, 1020, (char*)blockdigeststr);
							strcat_s(sendpkg.msg, 1020, " ");
							_itoa_s(whichblocknum, tempitoa, 10);
							strcat_s(sendpkg.msg, 1020, tempitoa);
							strcat_s(sendpkg.msg, 1020, " ");
							_itoa_s(i, tempitoa, 10);
							strcat_s(sendpkg.msg, 1020, tempitoa);
							strcat_s(sendpkg.msg, 1020, " ");
							strcat_s(sendpkg.msg, 1020, (char *)filedigeststr);
							strcat_s(sendpkg.msg, 1020, " ");
							strcat_s(sendpkg.msg, 1020, recvpkg.msg);
							sendServerPackage(&sendpkg);
							recvServerPackage(&recvpkg);

							printf("\n");
							printf("server response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
							printf("\n");

							// 第3步：
							// 把文件块传给DataNode
							// DN也要有个SQLite存文件信息，一块发了就完事
							// BLOCKHASH	TEXT	char * blockdigest
							// BLOCKNUM		INT		int	   whichblocknum
							// DUPLICATE	INT		int	   i
							// FILEHASH		TEXT	char * filedigest

							DNBlock * dnblock;
							dnblock = (DNBlock*)malloc(sizeof(DNBlock));
							dnblock->dupnum = i;
							dnblock->blocklength = thisblocklength;
							dnblock->blocknum = whichblocknum;
							memcpy_s(dnblock->sha256blockdigest, 64, blockdigeststr, 64);
							memcpy_s(dnblock->sha256filedigest, 64, filedigeststr, 64);
							memcpy_s(dnblock->blockbinarycontent, 64 * 1024 * 1024, fileblock, thisblocklength);
							iResult = sendDNBlock(dnblock, dnsocket, thisblocklength);
							if (iResult != 0) {
								printf("\n");
								printf("sendDNBlock error");
								printf("\n");
							}

							recvDNCMDmsg(&recvpkg, dnsocket);
							// 完成i块了，输出块信息
							printf("\n");
							printf("DN response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
							printf("\n");

							closeDNSocket(dnsocket);
							dnsocket = NULL;
							free(dnblock);
							dnblock = NULL;
						}
						free(fileblock);
						fileblock = NULL;
					}
					// end for whichblocknum

					sendpkg.type = 128;
					// FULLNAME
					strcpy_s(sendpkg.msg, filepath);
					strcat_s(sendpkg.msg, " ");
					// FILEHASH
					strcat_s(sendpkg.msg, (char *)filedigeststr);
					strcat_s(sendpkg.msg, " ");
					char tempitoa[10];
					ZeroMemory(tempitoa, 10);
					_itoa_s(devideblocknums, tempitoa, 10);
					strcat_s(sendpkg.msg, (char *)tempitoa);
					ZeroMemory(tempitoa, 10);
					_itoa_s(BLOCKDUPNUM, tempitoa, 10);
					strcat_s(sendpkg.msg, " ");
					strcat_s(sendpkg.msg, (char *)tempitoa);

					sendServerPackage(&sendpkg);
					ZeroMemory(sendpkg.msg, 1020);
					recvServerPackage(&recvpkg); // 收到一个128空包

				}
				fclose(uploadfile);
			}
			else {
				printf("\n");
				printf("USAGE: upload [filename]");
				printf("\n");
			}
		}

		else if (strcmp(temp, "fetch") == 0) {
			char * filename = strtok_s(temp_next, " ", &temp_next);
			if (filename != NULL && strcmp(filename, "") != 0) {
				FILE *fetchfile = NULL;
				errno_t err;
				err = fopen_s(&fetchfile, filename, "wb");
				if (err != 0) {
					printf("\n");
					printf("fetch file create fail!");
					printf("\n");
				}

				// Step 1:
				sendpkg.type = 122;
				strcpy_s(sendpkg.msg, "Print working directory");
				sendServerPackage(&sendpkg);
				ZeroMemory(sendpkg.msg, 1020);
				recvServerPackage(&recvpkg);
				printf("\n");
				printf("response msg type %d, msg: %s", recvpkg.type, recvpkg.msg);
				printf("\n");
				char filepath[1024];
				ZeroMemory(filepath, 1024);
				strcpy_s(filepath, recvpkg.msg);
				// 给路径名接上文件名
				strcat_s(filepath, filename);

				unsigned char filedigest[33];
				ZeroMemory(filedigest, sizeof(filedigest));
				mbedtls_sha256((unsigned char *)filepath, strlen(filepath), filedigest, 0);

				printf("file SHA256 digest:\n");
				for (int i = 0; i < 32; i++) {
					printf("%02x", filedigest[i]);
				}
				printf("\n");
				char filedigeststr[64 + 1];
				ZeroMemory(filedigeststr, 0);
				for (int i = 0; i < 32; i++) {
					char singlebuffer[2 + 1];
					sprintf_s(singlebuffer, "%02x", filedigest[i]);
					memcpy_s(filedigeststr + i * 2, 64, singlebuffer, 2);
				}
				filedigeststr[64] = '\0';

				sendpkg.type = 130;
				strcpy_s(sendpkg.msg, filedigeststr);
				sendServerPackage(&sendpkg);
				ZeroMemory(sendpkg.msg, 1020);
				// 服务器返回块总数
				recvServerPackage(&recvpkg);
				char * temp_next_recv;
				char * blocksnum_str = strtok_s(recvpkg.msg, " ", &temp_next_recv);
				char * duplicatenum_str = strtok_s(temp_next_recv, " ", &temp_next_recv);
				int blocksnum = atoi(blocksnum_str);
				int duplicatenum = atoi(duplicatenum_str);

				char * blockrecvbuf;
				blockrecvbuf = (char *)malloc(64 * 1024 * 1024);
				for (int i = 1; i <= blocksnum; i++) {
					ZeroMemory(blockrecvbuf, 64 * 1024 * 1024);
					ZeroMemory(sendpkg.msg, 1020);
					bool success = false;
					for (int currentduplicatenum = 1; currentduplicatenum <= duplicatenum; currentduplicatenum++) {
						// Step 2:
						sendpkg.type = 131;
						// 131包结构:第几个包、文件哈希、第几个副本
						char ibuf[20];
						ZeroMemory(ibuf, 20);
						_itoa_s(i, ibuf, 10);
						strcpy_s(sendpkg.msg, ibuf);
						strcat_s(sendpkg.msg, " ");
						strcat_s(sendpkg.msg, filedigeststr);
						strcat_s(sendpkg.msg, " ");
						ZeroMemory(ibuf, 20);
						_itoa_s(currentduplicatenum, ibuf, 10);
						strcat_s(sendpkg.msg, ibuf);
						strcat_s(sendpkg.msg, " ");
						sendServerPackage(&sendpkg);
						ZeroMemory(recvpkg.msg, 1020);
						recvServerPackage(&recvpkg);

						// Step 3:
						// 这之后服务器返回的就是该副本数对应的ip，就ok了
						char * blockhash = strtok_s(recvpkg.msg, " ", &temp_next_recv);
						char * blockDNip = strtok_s(temp_next_recv, " ", &temp_next_recv);

						SOCKET dnsocket;
						int DNconnectiResult = buildConntoDN(blockDNip, dnsocket);
						// 如果这一步失败的话，说明这个datanode不存活，需要找server再获取一个副本的地址
						if (DNconnectiResult != 0) {
							printf("\n");
							printf("Trying to fetch block from #%d back-up DataNode......", currentduplicatenum);
							printf("\n");
							printf("Total we've got %d back-up DataNodes......", duplicatenum);
							printf("\n");
							continue;
						}
						sendpkg.type = 132;
						strcpy_s(sendpkg.msg, filedigeststr);
						strcat_s(sendpkg.msg, "_");
						strcat_s(sendpkg.msg, blockhash);


						sendDNCMDmsg(&sendpkg, dnsocket);
						ZeroMemory(sendpkg.msg, 1020);

						ZeroMemory(recvpkg.msg, 1020);
						recvDNCMDmsg(&recvpkg, dnsocket);
						int blocksize = atoi(recvpkg.msg);

						int r = recv(dnsocket, blockrecvbuf, blocksize, 0);
						if (r == SOCKET_ERROR) {
							printf("Recv failed with error: %d\n", WSAGetLastError());
							break;
						}
						// 写入文件
						fwrite(blockrecvbuf, blocksize, 1, fetchfile);
						success = true;
						break;
					}
					if (!success) {
						printf("\n");
						printf("FAIL: fetching block#%d, ALL duplicate blocks are UNAVALIABLE.", i);
						printf("\n");
						printf("stop fetching file %s", filename);
						printf("\n");
						break;
					}
				}
				free(blockrecvbuf);
				blockrecvbuf = NULL;
				fclose(fetchfile);
			}
			else {
				printf("\n");
				printf("invalid filename!");
				printf("\n");
				break;
			}
		}

		else if (strcmp(temp, "help") == 0) {
			printf("\n '<dir>' parameter is must");
			printf("\n '[dir]' parameter is optional");
			printf("\n COMMAND            USAGE");
			printf("\n -----------------------------------------------");
			printf("\n status             show current status");
			printf("\n heartbeat [ip]     heartbeat server");
			printf("\n disconnect [ip]    disconnect with server");
			printf("\n");
			printf("\n dnlist             show datanode list");
			printf("\n dnrefresh [ip]     send heartbeat to datanode");
			printf("\n dnadd <ip>         add datanode");
			printf("\n dnremove <ip>      remove datanode");
			printf("\n dnconall           connect ALL datanodes");
			printf("\n");
			printf("\n ls                 list sub directory");
			printf("\n pwd                print working directory");
			printf("\n cd <dir>           change directory");
			printf("\n mkdir <dir>        make directory");
			printf("\n rm <dir/file>      remove directory/file");
			printf("\n mv <from> <to>     directory/file to directory");
			printf("\n upload <file>      upload file");
			printf("\n fetch <file>       fetch file");
		}
		else {
			printf("\n no such command: %s.", temp);
			printf("\n input 'help' for all commands avaliable.");
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

void keepHeartBeat(void * gapduration)
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
		Sleep((int)(gapduration) * 1000);
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

int closeDNSocket(SOCKET &socket) {
	iResult = shutdown(socket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(socket);
		WSACleanup();
		return 1;
	}
	closesocket(socket);
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
	iResult = getaddrinfo(argv[1], PORT_SERVER, &hints, &result);
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

int buildConntoDN(char *ip, SOCKET & DnConnectSocket)
{
	struct addrinfo *dnresult = NULL;
	struct addrinfo *dnptr = NULL;
	struct addrinfo	dnhints;
	int idnResult;

	ZeroMemory(&dnhints, sizeof(hints));
	dnhints.ai_family = AF_UNSPEC;
	dnhints.ai_socktype = SOCK_STREAM;
	dnhints.ai_protocol = IPPROTO_TCP;

	idnResult = getaddrinfo(ip, PORT_DN, &dnhints, &dnresult);
	if (idnResult != 0) {
		printf("getaddrinfo failed with error: %d\n", idnResult);
		return 1;
	}

	for (dnptr = dnresult; dnptr != NULL; dnptr = dnptr->ai_next) {
		DnConnectSocket = socket(dnptr->ai_family, dnptr->ai_socktype, dnptr->ai_protocol);
		if (DnConnectSocket == INVALID_SOCKET) {
			printf("DN socket failed with error: %ld\n", WSAGetLastError());
			return 1;

		}
		idnResult = connect(DnConnectSocket, dnptr->ai_addr, (int)dnptr->ai_addrlen);
		if (idnResult == SOCKET_ERROR) {
			closesocket(DnConnectSocket);
			DnConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(dnresult);

	if (DnConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to DN!\n");
		return 1;
	}
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
	sendbuf = NULL;
	return 0;
}

int sendDNBlock(DNBlock *msg, SOCKET targetSocket, int length) {
	char *sendbuf = (char *)malloc(sizeof(DNBlock));

	char *currentAddr = sendbuf;
	memcpy_s(sendbuf, sizeof(int), &msg->dupnum, sizeof(int));
	sendbuf += sizeof(int);
	memcpy_s(sendbuf, sizeof(int), &msg->blocknum, sizeof(int));
	sendbuf += sizeof(int);
	memcpy_s(sendbuf, sizeof(int), &msg->blocklength, sizeof(int));
	sendbuf += sizeof(int);
	memcpy_s(sendbuf, 64*sizeof(char), &msg->sha256blockdigest, 64 * sizeof(char));
	sendbuf += 64*sizeof(char);
	memcpy_s(sendbuf, 64 * sizeof(char), &msg->sha256filedigest, 64 * sizeof(char));
	sendbuf += 64*sizeof(char);
	memcpy_s(sendbuf, length * sizeof(char), &msg->blockbinarycontent, length * sizeof(char));
	sendbuf += length*sizeof(char);

	int r = send(targetSocket, currentAddr, sendbuf-currentAddr, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	free(currentAddr);
	sendbuf = NULL;
	currentAddr = NULL;
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

int sendDNCMDmsg(CMDmsg *msg, SOCKET &socket) {
	char *sendbuf = (char *)malloc(1024 * sizeof(char));
	int temp = sizeof(msg->type) + strlen(msg->msg);
	memcpy_s(sendbuf, 1024 * sizeof(char), &msg->type, sizeof(msg->type));
	strcpy_s(sendbuf + sizeof(msg->type), 1024 * sizeof(char) - sizeof(msg->type), msg->msg);

	int r = send(socket, sendbuf, temp, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	free(sendbuf);
	sendbuf = NULL;
	return 0;
}

int recvDNCMDmsg(CMDmsg *recvpkg, SOCKET &socket) {
	char recvbuf[1024];
	ZeroMemory(recvbuf, sizeof(recvbuf));
	int r = recv(socket, recvbuf, DEFAULT_BUFLEN, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	char *currentAddr = recvbuf;
	memcpy_s(&recvpkg->type, sizeof(recvpkg->type), currentAddr, sizeof(recvpkg->type));
	currentAddr += sizeof(recvpkg->type);
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