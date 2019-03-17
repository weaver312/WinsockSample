#include "pch.h"
#include "DataNode.h"

WSADATA wsaData;
int g_exit = 0;
char CTIME[24];
SOCKET ConnectClient = INVALID_SOCKET;
SOCKET DSSocket = INVALID_SOCKET;
SOCKET DCSocket = INVALID_SOCKET;
int DSSocketStatus = 0;
int DCSocketStatus = 0;
SOCKET ConnectServer = INVALID_SOCKET;
char recvbuf[SERVER_BUFLEN];
int recvbuflen = SERVER_BUFLEN;
int iResult = 0;
sqlite3 *blocksdb;

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("\nno target address!\n");
		printf("usage(example): datanode 192.168.1.1\n");
		return 1;
	}
	
	getCurrentTime();
	printf("%s DataNode starting ...\n", CTIME);

	if (_access("block.db", 0)) {
		iResult = sqlite3_open("block.db", &blocksdb);
		getCurrentTime();
		printf("%s Can't open blocksdb: %s\n", CTIME, sqlite3_errmsg(blocksdb));
		char *sql = (char *)
			"CREATE TABLE BLOCKS("  \
			"BLOCKHASH	TEXT    PRIMARY KEY NOT NULL," \
			"DUPLICATE  INT     NOT NULL, " \
			"BLOCKNUM	INT     NOT NULL," \
			"BLOCKLENGTH	INT     NOT NULL," \
			"FILEHASH	TEXT	NOT NULL);";
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
		iResult = sqlite3_open("block.db", &blocksdb);
	}


	iResult = init(argv[1]);
	if (iResult != 0) {
		getCurrentTime();
		printf("%s Initialize error!", CTIME);
		return 1;
	}
	
	// 启动监听Server的线程
	_beginthread(acceptServerIncoming, 0, (void *)ConnectServer);

	// 启动监听Client的线程
	_beginthread(acceptClientIncoming, 0, (void *)ConnectClient);

	while (!g_exit);
	// cleanup
	closesocket(ConnectServer);
	closesocket(ConnectClient);
	WSACleanup();

	return 0;
}


int init(char *& targetaddr)
{
	// 初始化 Winsock
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}


	iResult = prepareServerSocket(targetaddr);
	if (iResult == 1) {
		getCurrentTime();
		printf("%s PrepareServerSocket error!\n", CTIME);
		return 1;
	}

	iResult = prepareClientSocket(targetaddr);
	if (iResult == 1) {
		getCurrentTime();
		printf("%s PrepareClientSocket error!\n", CTIME);
		return 1;
	}

	printf("init success\n");
	return 0;
}

//开始监听Server
int prepareServerSocket(char *& targetaddr)
{
	struct addrinfo *DataNodeResult = NULL, *ptr = NULL, hints;
	int DataNodeiResult = 0;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// 解析服务器地址和端口
	DataNodeiResult = getaddrinfo(targetaddr, SERVER_PORT, &hints, &DataNodeResult);
	if (DataNodeiResult != 0) {
		printf("getaddrinfo failed with error: %d\n", DataNodeiResult);
		WSACleanup();
		return 1;
	}
	printf("server getaddrinfo success\n");

	// 根据物理参数创建一个监听Socket
	ConnectServer = socket(DataNodeResult->ai_family, DataNodeResult->ai_socktype, DataNodeResult->ai_protocol);
	if (ConnectServer == INVALID_SOCKET) {
		getCurrentTime();
		printf("%s ConnectServer create failed with error: %ld\n", CTIME, WSAGetLastError());
		freeaddrinfo(DataNodeResult);
		WSACleanup();
		return 1;
	}
	printf("server listenSocket success\n");

	// 3. 绑定一个监听socket到物理socket上，clientresult就是物理socket的一些参数
	// 绑定监听socket到物理socket上
	DataNodeiResult = bind(ConnectServer, DataNodeResult->ai_addr, (int)DataNodeResult->ai_addrlen);
	if (ConnectServer == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s ConnectServer bind failed with error: %d\n", CTIME, WSAGetLastError());
		freeaddrinfo(DataNodeResult);
		closesocket(ConnectServer);
		WSACleanup();
		return 1;
	}
	printf("server Socketbind success\n");
	// 所谓的物理socket，其实就是创建socket对象的一个中介信息，当创建socket对象的行为顺利完成，
	// 也就不需要前面准备的物理socket信息了
	// 所以这里可以释放掉这个"物理的"socket address info
	freeaddrinfo(DataNodeResult);

	// 4. 开启监听接口，至此初始化完成
	DataNodeiResult = listen(ConnectServer, SOMAXCONN);
	if (DataNodeiResult == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s ConnectServer listen failed with error: %d\n", CTIME, WSAGetLastError());
		closesocket(ConnectServer);
		WSACleanup();
		return 1;
	}
	printf("server SocketListen success\n");
	getCurrentTime();
	printf("%s ConnectServer initialized, iResult is %d.\n", CTIME, DataNodeiResult);
	return 0;
}

//开始监听Client
int prepareClientSocket(char *& targetaddr)
{
	struct addrinfo *DataNodeResult = NULL, *ptr = NULL, hints;
	int DataNodeiResult = 0;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// 解析服务器地址和端口
	DataNodeiResult = getaddrinfo(targetaddr, CLIENT_PORT, &hints, &DataNodeResult);
	if (DataNodeiResult != 0) {
		printf("getaddrinfo failed with error: %d\n", DataNodeiResult);
		WSACleanup();
		return 1;
	}
	printf("getaddrinfo success\n");

	// 根据物理参数创建一个监听Socket
	ConnectClient = socket(DataNodeResult->ai_family, DataNodeResult->ai_socktype, DataNodeResult->ai_protocol);
	if (ConnectClient == INVALID_SOCKET) {
		getCurrentTime();
		printf("%s ConnectClient create failed with error: %ld\n", CTIME, WSAGetLastError());
		freeaddrinfo(DataNodeResult);
		WSACleanup();
		return 1;
	}
	printf("client listenSocket success\n");

	// 3. 绑定一个监听socket到物理socket上，clientresult就是物理socket的一些参数
	// 绑定监听socket到物理socket上
	DataNodeiResult = bind(ConnectClient, DataNodeResult->ai_addr, (int)DataNodeResult->ai_addrlen);
	if (ConnectClient == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s ConnectClient bind failed with error: %d\n", CTIME, WSAGetLastError());
		freeaddrinfo(DataNodeResult);
		closesocket(ConnectClient);
		WSACleanup();
		return 1;
	}
	printf("client Socketbind success\n");
	// 所谓的物理socket，其实就是创建socket对象的一个中介信息，当创建socket对象的行为顺利完成，
	// 也就不需要前面准备的物理socket信息了
	// 所以这里可以释放掉这个"物理的"socket address info
	freeaddrinfo(DataNodeResult);

	// 4. 开启监听接口，至此初始化完成
	DataNodeiResult = listen(ConnectClient, SOMAXCONN);
	if (DataNodeiResult == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s ConnectClient listen failed with error: %d\n", CTIME, WSAGetLastError());
		closesocket(ConnectClient);
		WSACleanup();
		return 1;
	}
	printf("client SocketListen success\n");
	getCurrentTime();
	printf("%s ConnectServer initialized, iResult is %d.\n", CTIME, DataNodeiResult);
	return 0;
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

// 将Server加入服务线程列表
void acceptServerIncoming(void *ListenSocket)
{
	printf("server thread accept...\n");
	while (true) {
		DSSocket = accept((SOCKET)ListenSocket, 0, 0);
		printf("\n");
		printf("server accept() block canceled.");
		printf("\n");
		if (DSSocket == INVALID_SOCKET) {
			DSSocketStatus = 0;
			getCurrentTime();
			printf("%s Server accept failed with error: %d\n", CTIME, WSAGetLastError());
		}
		else {
			DSSocketStatus = 1;
			_beginthread(processServer, 0, NULL);
			getCurrentTime();
			printf("%s Server accept success.\n", CTIME);
		}
	}
}

// 将Client加入服务线程列表
void acceptClientIncoming(void *ListenSocket) 
{
	printf("client thread accept...\n");
	while (true) {
		DCSocket = accept((SOCKET)ListenSocket, 0, 0);
		printf("\n");
		printf("client accept() block canceled.");
		printf("\n");
		if (DCSocket == INVALID_SOCKET) {
			DCSocketStatus = 0;
			getCurrentTime();
			printf("%s Client accept failed with error: %d\n", CTIME, WSAGetLastError());
		}
		else {
			DCSocketStatus = 1;
			_beginthread(processClient, 0, NULL);
			getCurrentTime();
			printf("%s Client accept success.\n", CTIME);
		}
	}
}

// 处理Server发送来的包
void processServer(void *) 
{
	printf("processServer...");
	char recvbuf[SERVER_BUFLEN];
	int recvbuflen = SERVER_BUFLEN;
	CMDmsg recvpkg;
	bool connected = true;
	int RecviResult;
	while (connected) {
		if (DSSocketStatus == 0)
			continue;

		ZeroMemory(recvbuf, sizeof(recvbuf));

		// If no byte received, current thread will be blocked in recv func.
		// If no error occurs, recv returns the number of bytes received and
		//the buffer pointed to by the buf parameter will contain this data received.
		RecviResult = recv(DSSocket, recvbuf, recvbuflen, 0);

		// 结构体各域不一定连续，要分开赋值，且用secure函数
		char *currentAddr = recvbuf;
		memcpy_s(&recvpkg.type, sizeof(recvpkg.type), currentAddr, sizeof(recvpkg.type));
		currentAddr += sizeof(recvpkg.type);
		memcpy_s(recvpkg.msg, sizeof(recvpkg.msg), currentAddr, sizeof(recvpkg.msg));

		if (RecviResult > 0) {
			getCurrentTime();
			printf("%s Server Message received: type%d - %s\n", CTIME, recvpkg.type, recvpkg.msg);

			CMDmsg *sendpkg = (CMDmsg *)malloc(sizeof(CMDmsg));
			sendpkg->type = 1;
			ZeroMemory(sendpkg->msg, 1020);

			switch (recvpkg.type) {
			// heartbeat response
			case 111: {
				sendpkg->type = 111;
				strcpy_s(sendpkg->msg, "DN response : I am working hard");
				sendServerPackage(sendpkg);
				break;
			}
			default: {
				break;
			}
			}
			// clear-up
			free(sendpkg);
			sendpkg = NULL;
			ZeroMemory(recvbuf, sizeof(recvbuf));
		}
		// If the connection has been gracefully closed, the return value is zero.
		else if (RecviResult == 0) {
			getCurrentTime();
			printf("%s Server Connection closing...\n", CTIME);
			connected = false;
		}

		// Otherwise, a value of SOCKET_ERROR is returned, and a specific error code can be retrieved by calling WSAGetLastError.
		else {
			getCurrentTime();
			printf("%s Exception happened and server is closed. iResult is %d and WSAerror is %d.\n", CTIME, iResult, WSAGetLastError());
			connected = false;
		}
	}
	//关闭服务线程的顺序：
	//1.关闭本端作为发送方的端口
	iResult = shutdown(DSSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s Server : try to stop sender with WSAerror: %d\n", CTIME, WSAGetLastError());
	}
	printf("%s  Server just shut down.\n", CTIME);
	//2.彻底注销socket
	closesocket(DSSocket);
}

void processClient(void *)
{
	printf("processServer...");
	DNBlock *recvBlock = (DNBlock *)malloc(sizeof(DNBlock));
	CMDmsg *recvMsg = (CMDmsg *)malloc(sizeof(CMDmsg));
	CMDmsg *sendMsg = (CMDmsg *)malloc(sizeof(CMDmsg));
	char *recvbuf = (char *)malloc(sizeof(DNBlock));
	char *sendbuf = (char *)malloc(64 * 1024 * 1024 * sizeof(char));
	int recvbuflen = sizeof(DNBlock);
	int temp;
	int RecviResult;
	ZeroMemory(recvbuf, recvbuflen);
	ZeroMemory(sendMsg->msg, 1020);
	sendMsg->type = 0;

	// If no byte received, current thread will be blocked in recv func.
	// If no error occurs, recv returns the number of bytes received and
	//the buffer pointed to by the buf parameter will contain this data received.
	RecviResult = recv(DCSocket, recvbuf, recvbuflen, 0);
	if (RecviResult > 0) 
	{
		// 结构体各域不一定连续，要分开赋值，且用secure函数
		char *currentAddr = recvbuf;
		memcpy_s(&temp, sizeof(int), currentAddr, sizeof(int));
		currentAddr += sizeof(int);
		if (temp > 10) {
			recvMsg->type = temp;
			memcpy_s(recvMsg->msg, sizeof(recvMsg->msg), currentAddr, sizeof(recvMsg->msg));
			getCurrentTime();
			printf("%s Client Message received: type%d - %s\n", CTIME, recvMsg->type, recvMsg->msg);

			switch (recvMsg->type) {
				// heartbeat response
				case 111:
				{
					sendMsg->type = 111;
					strcpy_s(sendMsg->msg, "DN response : I am working hard");
					sendServerPackage(sendMsg);
					break;
				}
				case 132:
				{
					char filename[130];

					strcpy_s(filename, recvMsg->msg);
					filename[129]='\0';
					printf("prepare file ,filename is %s \n", filename);
					FILE *read;
					errno_t err = fopen_s(&read, filename, "rb");
					if (read == NULL || err != 0) {
						printf("\n");
						printf("file not exist!");
						printf("\n");
					}
					fseek(read, 0, SEEK_SET);
					fseek(read, 0, SEEK_END);
					int blockbytenum = ftell(read);
					char *blockbytenum_str=(char *)malloc(sizeof(char)*10);
					_itoa_s(blockbytenum, blockbytenum_str, 10, 10);
					ZeroMemory(sendMsg->msg, 1020);
					strcpy_s(sendMsg->msg, blockbytenum_str);
					sendClientCMDmsg(sendMsg, DCSocket);
					fseek(read, 0, SEEK_SET);
					fread(sendbuf, blockbytenum, 1, read);
					int r = send(DCSocket, sendbuf, blockbytenum, 0);
					if (r == SOCKET_ERROR) {
						printf("Send failed with error: %d\n", WSAGetLastError());
						break;
					}
					fclose(read);
				}
				default:
				{
					break;
				}
			}
		}
		else 
		{
			recvBlock->dupnum = temp;
			memcpy_s(&recvBlock->blocknum, sizeof(recvBlock->blocknum), currentAddr, sizeof(recvBlock->blocknum));
			currentAddr += sizeof(recvBlock->blocknum);

			memcpy_s(&recvBlock->blocklength, sizeof(recvBlock->blocklength), currentAddr, sizeof(recvBlock->blocklength));
			currentAddr += sizeof(recvBlock->blocklength);

			memcpy_s(recvBlock->sha256blockdigest, sizeof(recvBlock->sha256blockdigest), currentAddr, sizeof(recvBlock->sha256blockdigest));
			currentAddr += sizeof(recvBlock->sha256blockdigest);

			memcpy_s(recvBlock->sha256filedigest, sizeof(recvBlock->sha256filedigest), currentAddr, sizeof(recvBlock->sha256filedigest));
			currentAddr += sizeof(recvBlock->sha256filedigest);

			memcpy_s(recvBlock->blockbinarycontent, sizeof(recvBlock->blockbinarycontent), currentAddr, recvBlock->blocklength);

			printf("Client Block received......\n");
			char blocknum_str[10], blocklength_str[10], dupnum_str[10], blockhash[65], filehash[65];
			_itoa_s(recvBlock->blocknum, blocknum_str, 10);
			_itoa_s(recvBlock->blocklength, blocklength_str, 10);
			_itoa_s(recvBlock->dupnum, dupnum_str, 10);
			memcpy_s(blockhash, 65, recvBlock->sha256blockdigest, 64);
			memcpy_s(filehash, 65, recvBlock->sha256filedigest, 64);
			*(blockhash + 64) = '\0';
			*(filehash + 64) = '\0';
			addtoblocksDB((char*)blockhash, (char*)blocknum_str, (char *)blocklength_str,
				(char *)dupnum_str, (char *)filehash);

			FILE *write=NULL;
			char filename[130];
			char *filename_temp=filename;
			memcpy_s(filename_temp, 64, recvBlock->sha256filedigest, 64);
			filename_temp += 64;
			*filename_temp = '_';
			filename_temp++;
			memcpy_s(filename_temp, 64, recvBlock->sha256blockdigest, 64);
			filename_temp += 64;
			*filename_temp = '\0';

			errno_t err = fopen_s(&write, filename, "wb");
			if (write == NULL || err != 0) {
				printf("\n");
				printf("file open error!");
				printf("\n");
			}
			fwrite(recvBlock->blockbinarycontent, recvBlock->blocklength, 1, write);
			fclose(write);

			ZeroMemory(sendMsg->msg, 1020);
			sendMsg->type = 301;
			strcpy_s(sendMsg->msg, "DN receive finished!!!");
			sendClientCMDmsg(sendMsg, DCSocket);
		}
		
	}
	free(recvBlock);
	free(recvMsg);
	free(recvbuf);
	free(sendMsg);
	free(sendbuf);
	//关闭服务线程的顺序：
	//1.关闭本端作为发送方的端口
	iResult = shutdown(DCSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		getCurrentTime();
		printf("%s Client : try to stop sender with WSAerror: %d\n", CTIME, WSAGetLastError());
	}
	closesocket(DCSocket);
	printf("%s  Client just shut down.\n", CTIME);
	//2.彻底注销socket
}
int sendServerPackage(CMDmsg *msg)
{
	// 这里用strlen是为了仅发送字符串部分，去掉多余长度。传值前要注意char[]最后的\0
	char *sendbuf = (char *)malloc(1024 * sizeof(char));
	int temp = sizeof(msg->type) + strlen(msg->msg);
	memcpy_s(sendbuf, 1024 * sizeof(char), &msg->type, sizeof(msg->type));
	strcpy_s(sendbuf + sizeof(msg->type), 1024 * sizeof(char) - sizeof(msg->type), msg->msg);

	int r = send(DSSocket, sendbuf, temp, 0);
	if (r == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		return 1;
	}
	free(sendbuf);
	return 0;
}
bool addtoblocksDB(char * blockdigest, char *whichblocknum_str,char *blocklength,
	char *duplicatenum_str, char *filedigest)
{
	char *zErrMsg = 0;
	int rc;
	char sql[1024];
	ZeroMemory(sql, 1024);

	// BLOCKHASH	TEXT	char * blockdigest
	// BLOCKNUM		INT		int	   whichblocknum
	// DUPLICATE	INT		int	   i
	// BLOCKLENGTH  INT     int    blocklength
	// FILEHASH		TEXT	char * filedigest

	strcpy_s(sql, "INSERT INTO BLOCKS(BLOCKHASH, BLOCKNUM, BLOCKLENGTH, DUPLICATE, FILEHASH) VALUES('");
	strcat_s(sql, blockdigest);
	strcat_s(sql, "',");
	strcat_s(sql, whichblocknum_str);
	strcat_s(sql, ",");
	strcat_s(sql, blocklength);
	strcat_s(sql, ",");
	strcat_s(sql, duplicatenum_str);
	strcat_s(sql, ",'");
	strcat_s(sql, filedigest);
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

int sendClientCMDmsg(CMDmsg *msg, SOCKET &socket) {
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
	return 0;
}