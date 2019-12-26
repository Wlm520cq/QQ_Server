#include <WinSock2.h>   //soket ������ͷ�ļ�
#include <iostream>
#include <Windows.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib")

/*
		���߳� + socket ��̵õ�һ������ʹ��
		�û���������߳�ͬ��  socket���  �ٽ���  ȫ�ֱ���
*/

#define MAX_CLNT 256
#define MAX_BUF_SIZE 256

//���е����ӵĿͻ��˵� socket
SOCKET clntSocks[MAX_CLNT];
//����
HANDLE hMutex;
//����������
int clntCnt = 0;

//���͸����еĿͻ���
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

//����ͻ������ӵĺ���
unsigned WINAPI HandleCln(void* arg)
{
	//1.�����̴߳��ݹ����Ĳ���
	SOCKET hClntSock = *((SOCKET*)arg);

	int iLen = 0, i;
	char szMsg[MAX_BUF_SIZE] = { 0 };
	//2.�������ݵĽ��ա�����  ѭ������
	//���յ��ͻ��˵�����
	while ((iLen = recv(hClntSock, szMsg, sizeof(szMsg), 0)) != 0)
	{
		SendMsg(szMsg, iLen);
	}
	//3.����ĳ���ͻ��� �Ͽ�����  ��Ҫ����Ͽ�������
	//ֻҪ�ǲ���ȫ�ֱ���  ����һ���µĿͻ���  �ͼ��� ������Դ����
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
	{
		if (hClntSock == clntSocks[i])
		{
			//��λ
			while (i++ < clntCnt)
			{
				clntSocks[i] = clntSocks[i + 1];
			}
			break;
		}
	}
	clntCnt--;
	printf("��ʱ���ӵ����%d: ", clntCnt);
	ReleaseMutex(hMutex);
	closesocket(hClntSock);
	return 0;
}

int main()
{
	//�����׽��ֿ�
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	HANDLE hThread;

	wVersionRequested = MAKEWORD(1, 1);
	// ��ʼ���׽��ֿ�
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
	//����һ���������  
	hMutex = CreateMutex(NULL, FALSE, NULL);
	// �½��׽���
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(9190);

	// ���׽��ֵ�����IP��ַ���˿ں�9190
	if (bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("bind ERRORnum = %d\n", GetLastError());
		return -1;
	}

	// ��ʼ����
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
		// ���տͻ�����  sockConn��ʱ���Ŀͻ�������
		SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrCli, &len);

		//ÿ��һ�����ӣ��������һ���̣߳�����һ�����ˣ�ά���ͻ��˵�����
		//ÿ��һ�����ӣ�ȫ������Ӧ�ü�һ����Ա�������������1
		//ֻҪ�ǲ���ȫ�ֱ���  ����һ���µĿͻ���  �ͼ��� ������Դ����
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