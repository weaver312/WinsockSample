#include "pch.h"
#include "Server.h"

WSADATA wsaData;
BOOL g_exit = FALSE;
char CTIME[24];
int iResult = 0;

// Client相关变量
int	   clientiResult;
int    freeclientportnum = MAX_CLIENT;
int	   ClientSocketStatus[MAX_CLIENT];
HANDLE ClientThread[MAX_CLIENT];
SOCKET ClientSocket[MAX_CLIENT];
SOCKET ClientListenSocket = INVALID_SOCKET;
struct addrinfo *clientresult = NULL;
struct addrinfo clienthints;

// DATANODE相关变量
int DN_alive = 0;
DataNode *datanodes;
DataNode *searchalivedatanodes;

char *filetreebuf;
FILE * filetreefp;
cJSON *jsonfiletree_root;
cJSON *jsoncurnode[MAX_CLIENT];
sqlite3 * blocksdb;
sqlite3 * filesdb;

// temp variable for DB
int blocksumnum = 0;
int duplicatenum = 0;
char tempdnip[17];
char blockhash[64 + 1];

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

	if (_access("blocks.db", 0)) {
		iResult = sqlite3_open("blocks.db", &blocksdb);
		getCurrentTime();
		printf("%s Can't open blocksdb: %s\n", CTIME, sqlite3_errmsg(blocksdb));
		char *sql = (char *)
			"CREATE TABLE BLOCKS("  \
			// 这里好像不应该用blockhash的primary key，否则多副本时，相同的BlockHash插入会失败
			"BLOCKHASH	TEXT    NOT NULL," \
		 "DUPLICATENUM  INT     NOT NULL, " \
			"BLOCKNUM	INT     NOT NULL," \
			"FILEHASH   TEXT    NOT NULL," \
			"DATANODEIP	TEXT	NOT NULL);";
		char *zErrMsg = 0;

		/* Execute SQL statement */
		iResult = sqlite3_exec(blocksdb, sql, NULL, 0, &zErrMsg);
		if (iResult != SQLITE_OK) {
			printf("SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			exit(0);
		}
	}
	else {
		getCurrentTime();
		printf("%s Open blocksdb: %s\n", CTIME, sqlite3_errmsg(blocksdb));
		iResult = sqlite3_open("blocks.db", &blocksdb);
	}

	if (_access("files.db", 0)) {
		iResult = sqlite3_open("files.db", &filesdb);
		getCurrentTime();
		printf("%s Can't open filesdb: %s\n", CTIME, sqlite3_errmsg(filesdb));
		char *sql = (char *)
			"CREATE TABLE FILES("  \
			// 这里同理，就也不写primary key了
			"FILEHASH TEXT    KEY NOT NULL," \
			"BLOCKSUM INT     NOT NULL," \
		"DUPLICATENUM INT     NOT NULL," \
			"FULLNAME TEXT	  NOT NULL);";
		char *zErrMsg = 0;

		/* Execute SQL statement */
		iResult = sqlite3_exec(filesdb, sql, NULL, 0, &zErrMsg);
		if (iResult != SQLITE_OK) {
			printf("SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			exit(0);
		}
	}
	else {
		getCurrentTime();
		printf("%s Open filedb: %s\n", CTIME, sqlite3_errmsg(filesdb));
		iResult = sqlite3_open("files.db", &filesdb);
	}

	iResult = initialize();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Initialize error!", CTIME);
		return 1;
	}

	// 启动监听Client的线程
	_beginthread(acceptClientIncoming, 0, (void *)ClientListenSocket);

	int i = 0;

	do {} while (!g_exit);

	getCurrentTime();
	printf("%s Server closing...\n", CTIME);
	printf("------------------------------------------\n");
	// _beginthread的业务函数返回时会自动调用_endthread.
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
	printf("%s Ready to save datanode IPs...\n", CTIME);
	iResult = saveparam();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading datanodes!\n", CTIME);
		return 1;
	}

	getCurrentTime();
	printf("%s Ready to save filetree to local...\n", CTIME);
	iResult = saveFileTree();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading filetree!\n", CTIME);
		return 1;
	}

	cJSON_Delete(jsonfiletree_root);

	getCurrentTime();
	printf("%s Cleanup-ing winsock lib...\n", CTIME);
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
	}
	else {
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
	strcpy_s(datanodes->ip, "");
	datanodes->next = NULL;

	iResult = prepareClientSocket();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s PrepareClientSocket error!\n", CTIME);
		return 1;
	}

	// 初始化datanodes列表
	getCurrentTime();
	printf("%s Ready to load datanode IPs...\n", CTIME);
	iResult = loadparams();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading datanodes!\n", CTIME);
		return 1;
	}

	// 初始化VFS树
	getCurrentTime();
	printf("%s Ready to load filetree from json...\n", CTIME);
	iResult = loadFileTree();
	if (iResult == 1) {
		getCurrentTime();
		printf("%s Error when loading filetree!\n", CTIME);
		return 1;
	}

	return 0;
}

// 将Client加入服务线程列表
void acceptClientIncoming(void *ListenSocket)
{
	while (!g_exit) {
		if (freeclientportnum == 0) continue;

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
	cJSON *jsonpwdpath[1024];
	int pwddepth = 0;
	jsonpwdpath[pwddepth] = jsonfiletree_root;

	while (!g_exit && connected) {
		if (ClientSocketStatus[myindex] == 0)
			continue;

		int iResult;
		ZeroMemory(recvbuf, sizeof(recvbuf));
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
			sendpkg->type = 1;
			ZeroMemory(sendpkg->msg, 1020);

			switch (recvpkg.type) {

				// heartbeat response
			case 100: {
				sendpkg->type = 100;
				strcpy_s(sendpkg->msg, "pong");
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // connect, 不存在,
			case 101: {
				sendpkg->type = 101;
				strcpy_s(sendpkg->msg, "connected with server.");
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // disconnect
			case 102: {
				connected = false;

				sendpkg->type = 102;
				strcpy_s(sendpkg->msg, "disconnected with server.");
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // dnlist-list all datanodes
			case 110: {
				sendpkg->type = 110;
				strcpy_s(sendpkg->msg, "");

				while (cur != NULL && reslen > 20) {
					reslen -= 20;
					strcat_s(sendpkg->msg, 1020, cur->ip);
					strcat_s(sendpkg->msg, 1020, ";");
					cur = cur->next;
				}

				sendClientPackage(sendpkg, myindex);

				break;
			}


					  // list all datanodes which are CONNECTED
			case 201: {
				sendpkg->type = 201;
				strcpy_s(sendpkg->msg, "");
				if (searchalivedatanodes == NULL)
					searchalivedatanodes = cur;

				bool founded = FALSE;
				while (searchalivedatanodes != NULL) {
					if (searchalivedatanodes->status == 1) {
						strcpy_s(sendpkg->msg, searchalivedatanodes->ip);
						searchalivedatanodes = searchalivedatanodes->next;
						founded = TRUE;
						break;
					}
					searchalivedatanodes = searchalivedatanodes->next;
				}

				// 没找到alive的datanode，从头再搜一遍
				if (!founded) {
					searchalivedatanodes = cur;
					while (searchalivedatanodes != NULL) {
						if (searchalivedatanodes->status == 1) {
							strcpy_s(sendpkg->msg, searchalivedatanodes->ip);
							strcat_s(sendpkg->msg, ";");
							searchalivedatanodes = searchalivedatanodes->next;
							break;
						}
						searchalivedatanodes = searchalivedatanodes->next;
					}
				}

				sendClientPackage(sendpkg, myindex);

				break;
			}

					  // dnrefresh-heartbeat all connected datanode
			case 111: {
				int i = 0;
				while (cur != NULL) {
					if (cur->status == 1) {
						// 给每个DN发心跳包
						i++;
						sendpkg->type = 111;
						strcpy_s(sendpkg->msg, "heartbeat from server");
						sendDNPackage(cur->dnsocket, sendpkg);

						recvDNCMDmsg(&recvpkg, cur->dnsocket);
						getCurrentTime();
						printf("%s datanode %s response type: %d, msg: %s.\n", CTIME, cur->ip, recvpkg.type, recvpkg.msg);
					}
					cur = cur->next;
				}

				// 回复Client
				sendpkg->type = 111;
				char buf[sizeof(i) + 20];
				_itoa_s(i, buf, 10);
				buf[sizeof(i)] = '\0';
				strcat_s(buf, " nodes heartbeat");
				strcpy_s(sendpkg->msg, buf);
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // dnadd-add a datanode
			case 112: {
				// 检查合法性

				// 添加到表里
				sendpkg->type = 112;
				if (addparam(recvpkg.msg, 0) == 0) {
					strcpy_s(sendpkg->msg, "add datanode success.");
				}
				else {
					strcpy_s(sendpkg->msg, "add datanode fail, or duplicate exists probably");
				}
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // dnremove-remove a datanode
			case 113: {
				// 检查合法性

				// 给DataNode发包，关闭指定的DataNode

				// 从表中移除
				sendpkg->type = 113;
				if (deleteparam(recvpkg.msg) == 0) {
					strcpy_s(sendpkg->msg, "delete datanode success.");
				}
				else {
					strcpy_s(sendpkg->msg, "delete datanode fail.");
				}
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // dncon带ip参数-try to connect a datanode，暂时不存在
			case 114: {
				DataNode *returndn = NULL;

				// 尝试获取dn的ip
				sendpkg->type = 114;
				if (fetchparam(recvpkg.msg, returndn) == 0) {
					_beginthread(connectSingleDataNode, 0, (void *)returndn);
				}
				else {
					strcpy_s(sendpkg->msg, "datanode not found on server, please add datanode first. ");
					strcat_s(sendpkg->msg, "server response: server is trying to connect a datanode");
				}
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // dncon-try to connect ALL datanodes
			case 115: {
				// 暂时只输dncon时才连接
				connectEachDataNodes();

				sendpkg->type = 115;
				strcpy_s(sendpkg->msg, "trying to connect all datanodes");
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // ls-list sub directory
			case 121: {
				cJSON *cur = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "foldername");

				if (cur != NULL && cJSON_IsString(cur) && cur->valuestring != NULL) {
					cJSON *folders = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "folders");
					cJSON *files = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "files");
					cJSON *folder = NULL;
					cJSON *file = NULL;
					int reslen = 1020;

					strcat_s(sendpkg->msg, 1020, "FOLDERS:");
					cJSON_ArrayForEach(folder, folders) {
						cJSON *foldername = cJSON_GetObjectItemCaseSensitive(folder, "foldername");
						reslen -= strlen(foldername->valuestring);
						reslen -= sizeof(";");
						if (reslen > 20 && foldername != NULL) {
							strcat_s(sendpkg->msg, 1020, foldername->valuestring);
							strcat_s(sendpkg->msg, 1020, ";");
						}
						else {
							break;
						}
					}
					strcat_s(sendpkg->msg, 1020, "FILES:");

					cJSON_ArrayForEach(file, files) {
						cJSON *filename = cJSON_GetObjectItemCaseSensitive(file, "filename");
						reslen -= strlen(filename->valuestring);
						reslen -= sizeof(";");
						if (reslen > 20 && filename != NULL) {
							strcat_s(sendpkg->msg, 1020, filename->valuestring);
							strcat_s(sendpkg->msg, 1020, ";");
						}
						else {
							break;
						}
					}
				}
				sendpkg->type = 121;
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // pwd-print working directory
			case 122: {
				sendpkg->type = 122;
				for (int i = 0; i <= pwddepth; i++) {
					strcat_s(sendpkg->msg, 1020, cJSON_GetObjectItemCaseSensitive
					(jsonpwdpath[i], "foldername")->valuestring);
					strcat_s(sendpkg->msg, 1020, "/");
				}
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // cd带参数-change directory
			case 123: {
				cJSON *folders = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "folders");
				cJSON *folder = NULL;
				int founded = 0;

				if (strcmp(recvpkg.msg, "..") == 0) {
					if (pwddepth == 0)
						;
					else {
						founded = 1;
						jsonpwdpath[pwddepth] = NULL;
						pwddepth--;
						jsoncurnode[myindex] = jsonpwdpath[pwddepth];
					}
				}
				else {
					cJSON_ArrayForEach(folder, folders) {
						if (strcmp(cJSON_GetObjectItemCaseSensitive(folder, "foldername")->valuestring, recvpkg.msg) == 0) {
							jsoncurnode[myindex] = folder;
							founded = 1;
							pwddepth++;
							jsonpwdpath[pwddepth] = folder;
							break;
						}
					}
				}

				sendpkg->type = 123;
				strcpy_s(sendpkg->msg, "DIR change result:");
				if (founded == 0) {
					strcat_s(sendpkg->msg, "fail");
				}
				else {
					strcat_s(sendpkg->msg, "success");
				}
				sendClientPackage(sendpkg, myindex);

				break;
			}

					  // mkdir带参数-make dir
			case 124: {
				cJSON *folders = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "folders");
				cJSON *folder = NULL;
				int founded = 0;

				cJSON_ArrayForEach(folder, folders) {
					if (strcmp(cJSON_GetObjectItemCaseSensitive(folder, "foldername")->valuestring, recvpkg.msg) == 0) {
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

					strcpy_s(sendpkg->msg, "makedir success.");
				}
				else {
					strcpy_s(sendpkg->msg, "makedir failed, duplicate foldername existed.");
				}
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // rm带参数-remove directory/file
			case 125: {
				cJSON *folders = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "folders");
				cJSON *folder = NULL;
				int founded = 0;

				int folderindex = 0;
				cJSON_ArrayForEach(folder, folders) {
					if (strcmp(cJSON_GetObjectItemCaseSensitive(folder, "foldername")->valuestring, recvpkg.msg) == 0) {
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
					strcpy_s(sendpkg->msg, "dir remove success.");
				}
				else {
					strcpy_s(sendpkg->msg, "remove dir not found.");
				}
				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // 暂不实现mv带参数-move file to directory/file
			case 126: {
				break;
			}

					  // 暂不实现view带参数-view first 100bytes of file
			case 127: {
				break;
			}

					  // 未完成upload带参数-upload file
			case 128: {
				sendpkg->type = 128;

				/* 第1步 */
				char * temp_next = NULL;
				// temp是第1个字段，filename
				char * filename = strtok_s(recvpkg.msg, " ", &temp_next);
				// 第2个是sha256
				char * sha256 = strtok_s(temp_next, " ", &temp_next);
				char * blocksumnum = strtok_s(temp_next, " ", &temp_next);
				char * duplicatsumnum = strtok_s(temp_next, " ", &temp_next);

				cJSON * files = cJSON_GetObjectItemCaseSensitive(jsoncurnode[myindex], "files");
				cJSON * file = cJSON_CreateObject();

				cJSON_AddStringToObject(file, "filename", filename);
				cJSON_AddStringToObject(file, "filehash", sha256);

				cJSON_AddItemToArray(files, file);

				addtofilesDB(filename, blocksumnum, duplicatsumnum, sha256);

				// 通知Client执行后面两步
				strcpy_s(sendpkg->msg, "add file metadata to server success.");
				sendClientPackage(sendpkg, myindex);

				/* 2、3步都由Client执行 */

				/* 第2步给Client返回可用的DataNodes,201包 */

				/* 第3步把Block信息存起来,129包 */

				break;
			}
					  // upload block information
			case 129: {
				sendpkg->type = 129;
				// Server存信息
				// BLOCKHASH	TEXT	char * blockdigest
				// BLOCKNUM		INT		int	   whichblocknum
				// DUPLICATE	INT		int	   i
				// FILEHASH		TEXT	char * filedigest
				// DATANODEIP	TEXT	char * datanodeip
				char * temp_next = NULL;
				char atoibuf[10];
				ZeroMemory(atoibuf, 10);
				char * blockdigest = strtok_s(recvpkg.msg, " ", &temp_next);
				char * whichblocknum_str = strtok_s(temp_next, " ", &temp_next);
				char * duplicatenum_str = strtok_s(temp_next, " ", &temp_next);
				char * filedigest = strtok_s(temp_next, " ", &temp_next);
				char * datanodeip = strtok_s(temp_next, " ", &temp_next);

				// TODO:
				addtoblocksDB(blockdigest, whichblocknum_str, duplicatenum_str,
					filedigest, datanodeip);

				// 通知Client执行后面两步
				strcpy_s(sendpkg->msg, "block metadata saved:");
				strcat_s(sendpkg->msg, whichblocknum_str);
				strcat_s(sendpkg->msg, ",");
				strcat_s(sendpkg->msg, duplicatenum_str);
				strcat_s(sendpkg->msg, ",");
				strcat_s(sendpkg->msg, filedigest);
				strcat_s(sendpkg->msg, ",");
				strcat_s(sendpkg->msg, blockdigest);
				strcat_s(sendpkg->msg, ",");
				strcat_s(sendpkg->msg, datanodeip);

				sendClientPackage(sendpkg, myindex);
				break;
			}

					  // fetch-stage1 by filename
			case 130: {
				// 130包：fetch的哈希值
				// 取出总块数量并返回
				getfiledb_blocksumnum(recvpkg.msg);
				sendpkg->type = 130;
				char blockssumnum_buf[20];
				_itoa_s(blocksumnum, blockssumnum_buf, 10);
				strcpy_s(sendpkg->msg, blockssumnum_buf);
				strcat_s(sendpkg->msg, " ");
				ZeroMemory(blockssumnum_buf, 20);
				_itoa_s(duplicatenum, blockssumnum_buf, 10);
				strcat_s(sendpkg->msg, blockssumnum_buf);
				sendClientPackage(sendpkg, myindex);
				blocksumnum = 0;
				duplicatenum = 0;
				break;
			}
					  // fetch-stage2, 通过文件块的哈希和存放处的DNip和偏移量，
			case 131: {
				char * temp_next = NULL;
				char * blockseriesnum = strtok_s(recvpkg.msg, " ", &temp_next);
				char * filedigest = strtok_s(temp_next, " ", &temp_next);
				char * duplicatenum = strtok_s(temp_next, " ", &temp_next);
				getblocksdb_blockdigest_dnip(filedigest, blockseriesnum, duplicatenum);

				ZeroMemory(sendpkg->msg, 1020);
				sendpkg->type = 131;
				strcpy_s(sendpkg->msg, blockhash);
				strcat_s(sendpkg->msg, " ");
				strcat_s(sendpkg->msg, tempdnip);
				strcat_s(sendpkg->msg, " ");
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
/*
int prepareAllDNSocket() {

}
// 开始准备DN连接的socket
int prepareOneDNSocket(DataNode *node)
{
	int DNiResult;
	SOCKET DNListenSocket;
	ZeroMemory(&node->dnsockethints, sizeof(node->dnsockethints));
	node->dnsockethints.ai_family = AF_INET;
	node->dnsockethints.ai_socktype = SOCK_STREAM;
	node->dnsockethints.ai_protocol = IPPROTO_TCP;
	node->dnsockethints.ai_flags = AI_PASSIVE;
	DNiResult = getaddrinfo(node->ip, PORT_CLIENT, &(node->dnsockethints), &(node->dnsocketresult));
	if (DNiResult != 0) {
		getCurrentTime();
		printf("%s ip %s DNSocket get address info failed with error: %d\n", CTIME, node->ip, DNiResult);
		// WSACleanup();
		return 1;
	}
	addrinfo *ptr;
	for (ptr = node->dnsocketresult; ptr != NULL; ptr = ptr->ai_next) {
		node->dnsocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (node->dnsocket == INVALID_SOCKET) {
			printf("ip %s socket failed with error: %ld\n", node->ip, WSAGetLastError());
			// WSACleanup();
			return 1;
		}
		DNiResult = connect(node->dnsocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (DNiResult == SOCKET_ERROR) {
			closesocket(node->dnsocket);
			node->dnsocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	// 创建socket对象完成，不再需要socket
	freeaddrinfo(node->dnsocketresult);
	// 连接失败
	if (node->dnsocket == INVALID_SOCKET) {
		printf("ip %s Unable to connect to server!\n", node->dnsocket);
		// WSACleanup();
		return 1;
	}
	return 0;
}
*/

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

// 尝试连接链表每个ip
int connectEachDataNodes() {
	DataNode *cur = datanodes->next;
	int result;
	while (cur != NULL) {
		if (cur->status == 0) {
			getCurrentTime();
			printf("%s %s DN connecting\n", CTIME, cur->ip);
			result = _beginthread(connectSingleDataNode, 0, cur);
		}
		cur = cur->next;
	}
	return 0;
}

void connectSingleDataNode(void *curdn) {
	int dniResult;
	struct addrinfo dnhints;
	struct addrinfo *dnresult = NULL;
	struct addrinfo *dnptr = NULL;

	ZeroMemory(&dnhints, sizeof(dnhints));
	dnhints.ai_family = AF_UNSPEC;
	dnhints.ai_socktype = SOCK_STREAM;
	// 这里可以选用UDP等其他底层协议
	dnhints.ai_protocol = IPPROTO_TCP;

	dniResult = getaddrinfo(((DataNode *)curdn)->ip, PORT_DATANODE, &dnhints, &dnresult);
	if (dniResult != 0) {
		getCurrentTime();
		printf("%s %s ip getaddrinfo failed with error: %d\n", CTIME, ((DataNode *)curdn)->ip, dniResult);
		_endthread();
	}

	for (dnptr = dnresult; dnptr != NULL; dnptr = dnptr->ai_next) {
		((DataNode *)curdn)->dnsocket = socket(dnptr->ai_family, dnptr->ai_socktype, dnptr->ai_protocol);
		if (((DataNode *)curdn)->dnsocket == INVALID_SOCKET) {
			getCurrentTime();
			printf("%s %s ip  create failed with error: %ld\n", CTIME, ((DataNode *)curdn)->ip, WSAGetLastError());
			_endthread();
		}
		dniResult = connect(((DataNode *)curdn)->dnsocket, dnptr->ai_addr, (int)dnptr->ai_addrlen);
		if (dniResult == SOCKET_ERROR) {
			closesocket(((DataNode *)curdn)->dnsocket);
			((DataNode *)curdn)->dnsocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(dnresult);

	// 确认连接成功/失败
	if (((DataNode *)curdn)->dnsocket == INVALID_SOCKET) {
		getCurrentTime();
		printf("%s %s ip unable to connect datanode !\n", CTIME, ((DataNode *)curdn)->ip);
		_endthread();
	}
	getCurrentTime();
	printf("%s %s ip datanode connected!\n", CTIME, ((DataNode *)curdn)->ip);
	((DataNode *)curdn)->status = 1;
	_endthread();
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
				printf("%s %s DN disconnected", CTIME, cur->ip);
			}
			else {
				getCurrentTime();
				printf("%s %s DN disconnect failed", CTIME, cur->ip);
			}
		}
		cur = cur->next;
	}
	return 0;
}

// 关闭单个DataNode的连接
int disconnectSingleDataNode(DataNode * &curdn) {
	int result;

	result = shutdown(curdn->dnsocket, SD_SEND);
	if (result == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s %s ip shutdown failed with error: %d\n", CTIME, curdn->ip, WSAGetLastError());
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
	DataNode * begin = datanodes;
	// 创建新节点的临时结构体
	DataNode *temp = NULL;

	FILE *paramsfp = NULL;
	errno_t err;
	err = fopen_s(&paramsfp, PARAMFILE, "r");
	if (err != 0) {
		// 为了首次使用能自动创建配置文件，这里用w+模式打开
		fopen_s(&paramsfp, PARAMFILE, "w");
		fclose(paramsfp);
		// 并返回正常
		return 0;
	}
	// 如果未发生err而且fp还是NULL，那就是异常了
	if (paramsfp == NULL) {
		fclose(paramsfp);
		paramsfp = NULL;
		fopen_s(&paramsfp, PARAMFILE, "w");
		return 1;
	}

	else {
		char ipline[17];
		ZeroMemory(ipline, 17);
		while (!feof(paramsfp)) {
			if (fgets(ipline, 17, paramsfp) == NULL || strcmp(ipline, "") == 0 || strcmp(ipline, "\n") == 0)
				continue;
			else {
				printf("%s", ipline);
				//创建新节点
				temp = (DataNode *)malloc(sizeof(DataNode));
				strcpy_s(temp->ip, 17, ipline);
				// 替换掉换行符
				if (strcmp(&temp->ip[strlen(temp->ip) - 1], "\n") == 0)
					temp->ip[strlen(temp->ip) - 1] = '\0';
				temp->status = 0;
				temp->next = NULL;

				// 把新节点接到头的后面，并开始处理下一个节点
				begin->next = temp;
				begin = begin->next;
				begin->next = NULL;
			}
		}
		// 复原头结点地址，这步之后datanodes又是头结点了。头结点的status值是-1，其next才是第一个数据。
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
		if (strcmp(temp->ip, ip) == 0) {
			return 1;
		}
		temp = temp->next;
	}
	temp = NULL;

	temp = (DataNode *)malloc(sizeof(DataNode));
	strcpy_s(temp->ip, 16, ip);
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

	DataNode *temp = datanodes->next;
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
	char *sendbuf = (char *)malloc(1024 * sizeof(char));
	int temp = sizeof(msg->type) + strlen(msg->msg);
	memcpy_s(sendbuf, 1024 * sizeof(char), &msg->type, sizeof(msg->type));
	strcpy_s(sendbuf + sizeof(msg->type), 1024 * sizeof(char) - sizeof(msg->type), msg->msg);

	int r = send(ClientSocket[index], sendbuf, temp, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	free(sendbuf);
	return 0;
}

// 发送DataNode通信包
int sendDNPackage(SOCKET &dnsocket, CMDmsg *msg) {
	char *sendbuf = (char *)malloc(1024 * sizeof(char));
	int temp = sizeof(msg->type) + strlen(msg->msg);
	memcpy_s(sendbuf, 1024 * sizeof(char), &msg->type, sizeof(msg->type));
	strcpy_s(sendbuf + sizeof(msg->type), 1024 * sizeof(char) - sizeof(msg->type), msg->msg);

	int r = send(dnsocket, sendbuf, temp, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	return 0;
}

// 载入文件树，文件名由宏指定
int loadFileTree() {
	errno_t err;
	err = fopen_s(&filetreefp, FILETREEFILE, "r");
	if (err != 0 || filetreefp == NULL) {
		// 也可能是初次启动，没有树文件。刚好可以初始化一个
		fopen_s(&filetreefp, FILETREEFILE, "w");
		fclose(filetreefp);
		jsonfiletree_root = cJSON_CreateObject();
		// 根目录暂定foldername为空串
		cJSON_AddStringToObject(jsonfiletree_root, "foldername", "");
		cJSON_AddArrayToObject(jsonfiletree_root, "folders");
		cJSON_AddArrayToObject(jsonfiletree_root, "files");
		return 0;
	}

	fseek(filetreefp, 0, SEEK_END);
	int size = ftell(filetreefp);
	// 根据树大小动态申请空间
	filetreebuf = (char *)malloc(size + 1);
	filetreebuf[size] = '\0';

	fseek(filetreefp, 0L, SEEK_SET);
	fread_s(filetreebuf, size + 1, sizeof(char), size, filetreefp);
	fclose(filetreefp);
	printf("Size of json treebuf: %ld bytes.\n", size);

	printf("cJSON tree loading...\n");
	jsonfiletree_root = cJSON_Parse(filetreebuf);

	//printf("\n");
	//printf("%s", filetreebuf);
	//printf("\n");
	// free(filetreebuf);

	printf("cJSON tree load finished.\n");
	if (jsonfiletree_root == NULL) {
		// 可能文件就是空的，那直接建个空树就行
		if (size == 0) {
			jsonfiletree_root = cJSON_CreateObject();
			// 根目录暂定foldername为空串
			cJSON_AddStringToObject(jsonfiletree_root, "foldername", "");
			cJSON_AddArrayToObject(jsonfiletree_root, "folders");
			cJSON_AddArrayToObject(jsonfiletree_root, "files");
			return 0;
		}
		else {
			// 其余状况均可视为是读取出错
			printf("Exception happened when parsing filetree\n");
			const char *error_ptr = cJSON_GetErrorPtr();
			if (error_ptr != NULL)
				fprintf(stderr, "Error: %s\n", error_ptr);
			return 1;
		}
	}
	else {
		return 0;
	}
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

bool addtofilesDB(char *& filename, char *& blocksumnum, char *& duplicatenum, char *& sha256)
{
	char *zErrMsg = 0;
	int rc;
	char sql[1024];

	strcpy_s(sql, "INSERT INTO FILES(FILEHASH, BLOCKSUM, DUPLICATENUM, FULLNAME) "\
		"VALUES('");
	strcat_s(sql, sha256);
	strcat_s(sql, "',");
	strcat_s(sql, blocksumnum);
	strcat_s(sql, ",");
	strcat_s(sql, duplicatenum);
	strcat_s(sql, ",'");
	strcat_s(sql, filename);
	strcat_s(sql, "');");

	rc = sqlite3_exec(filesdb, sql, NULL, 0, &zErrMsg);
	printf("filedb SQL statement: %s\n", sql);
	if (rc != SQLITE_OK) {
		printf("file SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return false;
	}
	else {
		printf("file record insert successfully\n");
		return true;
	}
}

bool addtoblocksDB(char *& blockdigest, char *& whichblocknum_str,
	char *& duplicatenum_str, char *& filedigest, char *& datanodeip)
{
	char *zErrMsg = 0;
	int rc;
	char sql[1024];
	ZeroMemory(sql, 1024);

	// BLOCKHASH	TEXT	char * blockdigest
	// BLOCKNUM		INT		int	   whichblocknum
	// DUPLICATE	INT		int	   i
	// FILEHASH		TEXT	char * filedigest
	// DATANODEIP	TEXT	char * datanodeip

	strcpy_s(sql, "INSERT INTO BLOCKS(BLOCKHASH, BLOCKNUM, DUPLICATENUM, FILEHASH, DATANODEIP) "\
		"VALUES('");
	strcat_s(sql, blockdigest);
	strcat_s(sql, "',");
	strcat_s(sql, whichblocknum_str);
	strcat_s(sql, ",");
	strcat_s(sql, duplicatenum_str);
	strcat_s(sql, ",'");
	strcat_s(sql, filedigest);
	strcat_s(sql, "','");
	strcat_s(sql, datanodeip);
	strcat_s(sql, "');");

	rc = sqlite3_exec(blocksdb, sql, NULL, 0, &zErrMsg);
	printf("blockdb SQL statement: %s\n", sql);
	if (rc != SQLITE_OK) {
		printf("block SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return false;
	}
	else {
		printf("block record insert successfully\n");
		return true;
	}
}

int getblocksdb_blockdigest_dnip(char * filehash, char * blockseriesnum, char * duplicatenum) {
	char *zErrMsg = 0;
	int rc;
	char sql[1024];
	const char* data = "Callback2 function called";

	ZeroMemory(sql, 1024);
	//  "BLOCKHASH	TEXT    PRIMARY KEY NOT NULL," \
	//	"DUPLICATE  INT     NOT NULL, " \
	//	"BLOCKNUM	INT     NOT NULL," \
	//	"FILEHASH   TEXT    NOT NULL," \
	//	"DATANODEIP	TEXT	NOT NULL);";
	strcpy_s(sql, "SELECT BLOCKHASH,DATANODEIP FROM BLOCKS WHERE FILEHASH = '");
	strcat_s(sql, filehash);
	strcat_s(sql, "' AND BLOCKNUM = ");
	strcat_s(sql, blockseriesnum);
	strcat_s(sql, " AND DUPLICATENUM = ");
	strcat_s(sql, duplicatenum);
	strcat_s(sql, ";");


	rc = sqlite3_exec(blocksdb, sql, callback_blocklocation, (void*)data, &zErrMsg);
	if (rc != SQLITE_OK) {
		printf("getblockssumnum sqlite failed");
		printf("\n");
		printf("SQL statement: %s", sql);
		printf("\n");
		sqlite3_free(zErrMsg);
	}
	else {
		printf("getblockssumnum sqlite successed");
		printf("\n");
	}

	return 0;
}


void clear_dnip_blockhash() {
	ZeroMemory(tempdnip, 17);
	ZeroMemory(blockhash, 64 + 1);
}

int callback_blocklocation(void *data, int argc, char **argv, char **azColName) {
	if (argc == 0) {
		printf("error in callback2\n");
		return 1;
	}
	strcpy_s(blockhash, argv[0]);
	strcpy_s(tempdnip, argv[1]);
	return 0;
}

int getfiledb_blocksumnum(char * blockhash) {
	char *zErrMsg = 0;
	int rc;
	char sql[1024];
	const char* data = "Callback1 function called";

	ZeroMemory(sql, 1024);
	strcpy_s(sql, "SELECT BLOCKSUM,DUPLICATENUM FROM FILES WHERE FILEHASH = '");
	strcat_s(sql, blockhash);
	strcat_s(sql, "';");

	rc = sqlite3_exec(filesdb, sql, callback_blocksumnum, (void*)data, &zErrMsg);
	if (rc != SQLITE_OK) {
		printf("getblockssumnum sqlite failed");
		printf("\n");
		sqlite3_free(zErrMsg);
	}
	else {
		printf("getblockssumnum sqlite successed");
		printf("\n");
	}

	return 0;
}


int callback_blocksumnum(void *data, int argc, char **argv, char **azColName) {
	if (argc == 0) {
		printf("error in callback1\n");
		return 1;
	}
	blocksumnum = atoi(argv[0]);
	duplicatenum = atoi(argv[1]);
	return 0;
}

int recvDNCMDmsg(CMDmsg *recvpkg, SOCKET &socket) {
	char recvbuf[1024];
	ZeroMemory(recvbuf, sizeof(recvbuf));
	int r = recv(socket, recvbuf, DN_CMD_BUFLEN, 0);
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