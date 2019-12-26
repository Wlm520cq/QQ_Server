#include <WinSock2.h>
#include <iostream>
#include <Windows.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib")

#define NAME_SIZE 32
#define BUF_SIZE 256

char szName[NAME_SIZE] = "[DEFAULT]";
char szMsg[BUF_SIZE];

//接收服务端的消息
unsigned WINAPI RecvMsg(void* arg)
{
	//1.接收线程传递过来的参数
	SOCKET hClntSock = *((SOCKET*)arg);
	//有 名字 和 消息 的组合
	char szNameMsg[NAME_SIZE + BUF_SIZE];
	int iLen = 0;
	while (1)
	{
		iLen = recv(hClntSock, szNameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		//服务端 断开
		if (iLen==-1)
		{
			return-1;
		}
		//szNameMsg 的 0 到 ILen - 1 都是收到的数据 ILen个
		szNameMsg[iLen] = 0;
		//接收到的数据 输出到控制台
		fputs(szNameMsg, stdout);
	}
	return 0;
}

//发送消息给服务器
unsigned WINAPI SendMsg(void* arg)
{
	//1.接收线程传递过来的参数
	SOCKET hClntSock = *((SOCKET*)arg);
	//有 名字 和 消息 的组合
	char szNameMsg[NAME_SIZE + BUF_SIZE];
	//循环接收来自于控制台的参数
	while (1)
	{
		fgets(szMsg, BUF_SIZE, stdin);  //阻塞在这一句

		//退出机制  当收到 q 或 Q  退出
		if (!strcmp(szMsg,"Q\n") || !strcmp(szMsg,"q\n"))
		{
			closesocket(hClntSock);
			exit(0);
		}
 
		sprintf_s(szNameMsg, "%s %s", szName, szMsg);//字符串拼接
		send(hClntSock, szNameMsg, strlen(szNameMsg), 0);//发送
	}

	return 0;
}

//带参数的 main 函数，用命令行启动  在当前目录按下 shift + 鼠标右键 打开 cmd
int main(int argc, char* argv[])
{
	//加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	SOCKET hSock;
	SOCKADDR_IN servRdr;//服务器
	//hSendThread 发送  hRecvThread 接收
	HANDLE hSendThread, hRecvThread;

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

	sprintf_s(szName, "[%s]", argv);

	//1.建立 socket 
	hSock = socket(PF_INET, SOCK_STREAM, 0);

	//2.配置端口和地址
	memset(&servRdr, 0, sizeof(servRdr));
	servRdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	servRdr.sin_family = AF_INET;
	servRdr.sin_port = htons(9190);

	//3.连接服务器
	if (connect(hSock, (SOCKADDR*)&servRdr, sizeof(servRdr)) == SOCKET_ERROR)
	{
		printf("connect error code = %d \n",GetLastError());
		return -1;
	}

	//4.接收服务端的消息 开启一个线程接收
	hRecvThread=(HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

	//5.发送消息给服务端 开启一个线程发送消息
	hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);

	//等待内核对象的信号发生变化
	WaitForSingleObject(hSendThread, INFINITE);
	WaitForSingleObject(hRecvThread, INFINITE);

	//6.关闭套接字
	closesocket(hSock);

	WSACleanup();

	//system("pause");
	return 0;
}