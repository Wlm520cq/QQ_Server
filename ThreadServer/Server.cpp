#include <WinSock2.h>   //soket 网络编程头文件
#include <iostream>
#include <Windows.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib")

/*
		多线程 + socket 编程得到一个联合使用
		用互斥体进行线程同步  socket编程  临界区  全局变量
*/

#define MAX_CLNT 256
#define MAX_BUF_SIZE 256

//所有的连接的客户端的 socket
SOCKET clntSocks[MAX_CLNT];
//加锁
HANDLE hMutex;
//最大的连接数
int clntCnt = 0;

//发送给所有的客户端
void SendMsg(char* szMsg, int iLen)
{
	int i = 0;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
	{
		send(clntSocks[i], szMsg, iLen, 0);
	}
	ReleaseMutex(hMutex);
}

//处理客户端连接的函数
unsigned WINAPI HandleCln(void* arg)
{
	//1.接收线程传递过来的参数
	SOCKET hClntSock = *((SOCKET*)arg);

	int iLen = 0, i;
	char szMsg[MAX_BUF_SIZE] = { 0 };
	//2.进行数据的接收、发送  循环接收
	//接收到客户端的数据
	while ((iLen = recv(hClntSock, szMsg, sizeof(szMsg), 0)) != 0)
	{
		SendMsg(szMsg, iLen);
	}
	//3.处理某个客户端 断开连接  需要处理断开的连接
	//只要是操纵全局变量  创建一个新的客户端  就加锁 避免资源竞争
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
	{
		if (hClntSock == clntSocks[i])
		{
			//移位
			while (i++ < clntCnt)
			{
				clntSocks[i] = clntSocks[i + 1];
			}
			break;
		}
	}
	clntCnt--;
	printf("此时连接的舒服%d: ", clntCnt);
	ReleaseMutex(hMutex);
	closesocket(hClntSock);
	return 0;
}

int main()
{
	//加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	HANDLE hThread;

	wVersionRequested = MAKEWORD(1, 1);
	// 初始化套接字库
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return err;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return -1;
	}
	//创建一个互斥对象  
	hMutex = CreateMutex(NULL, FALSE, NULL);
	// 新建套接字
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(9190);

	// 绑定套接字到本地IP地址，端口号9190
	if (bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("bind ERRORnum = %d\n", GetLastError());
		return -1;
	}

	// 开始监听
	if (listen(sockSrv, 5) == SOCKET_ERROR)
	{
		printf("listen ERRORnum = %d\n", GetLastError());
		return -1;
	}

	printf("start listen\n");

	SOCKADDR_IN addrCli;
	int len = sizeof(SOCKADDR);

	while (1)
	{
		// 接收客户连接  sockConn此时来的客户端连接
		SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrCli, &len);

		//每来一个连接，服务端起一个线程（安排一个工人）维护客户端的连接
		//每来一个连接，全局数组应该加一个成员，最大连接数加1
		//只要是操纵全局变量  创建一个新的客户端  就加锁 避免资源竞争
		WaitForSingleObject(hMutex, INFINITE);
		clntSocks[clntCnt++] = sockConn;
		ReleaseMutex(hMutex);

		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleCln, (void*)&sockConn, 0, NULL);

		printf("Connect client IP: %s \n", inet_ntoa(addrCli.sin_addr));
		printf("Connect client num: %d \n", clntCnt);
	}

	closesocket(sockSrv);
	WSACleanup();


	system("pause");
	return 0;
}