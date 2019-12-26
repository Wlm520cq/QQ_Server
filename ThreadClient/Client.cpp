#include <WinSock2.h>
#include <iostream>
#include <Windows.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib")

#define NAME_SIZE 32
#define BUF_SIZE 256

char szName[NAME_SIZE] = "[DEFAULT]";
char szMsg[BUF_SIZE];

//���շ���˵���Ϣ
unsigned WINAPI RecvMsg(void* arg)
{
	//1.�����̴߳��ݹ����Ĳ���
	SOCKET hClntSock = *((SOCKET*)arg);
	//�� ���� �� ��Ϣ �����
	char szNameMsg[NAME_SIZE + BUF_SIZE];
	int iLen = 0;
	while (1)
	{
		iLen = recv(hClntSock, szNameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		//����� �Ͽ�
		if (iLen==-1)
		{
			return-1;
		}
		//szNameMsg �� 0 �� ILen - 1 �����յ������� ILen��
		szNameMsg[iLen] = 0;
		//���յ������� ���������̨
		fputs(szNameMsg, stdout);
	}
	return 0;
}

//������Ϣ��������
unsigned WINAPI SendMsg(void* arg)
{
	//1.�����̴߳��ݹ����Ĳ���
	SOCKET hClntSock = *((SOCKET*)arg);
	//�� ���� �� ��Ϣ �����
	char szNameMsg[NAME_SIZE + BUF_SIZE];
	//ѭ�����������ڿ���̨�Ĳ���
	while (1)
	{
		fgets(szMsg, BUF_SIZE, stdin);  //��������һ��

		//�˳�����  ���յ� q �� Q  �˳�
		if (!strcmp(szMsg,"Q\n") || !strcmp(szMsg,"q\n"))
		{
			closesocket(hClntSock);
			exit(0);
		}
 
		sprintf_s(szNameMsg, "%s %s", szName, szMsg);//�ַ���ƴ��
		send(hClntSock, szNameMsg, strlen(szNameMsg), 0);//����
	}

	return 0;
}

//�������� main ������������������  �ڵ�ǰĿ¼���� shift + ����Ҽ� �� cmd
int main(int argc, char* argv[])
{
	//�����׽��ֿ�
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	SOCKET hSock;
	SOCKADDR_IN servRdr;//������
	//hSendThread ����  hRecvThread ����
	HANDLE hSendThread, hRecvThread;

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

	sprintf_s(szName, "[%s]", argv);

	//1.���� socket 
	hSock = socket(PF_INET, SOCK_STREAM, 0);

	//2.���ö˿ں͵�ַ
	memset(&servRdr, 0, sizeof(servRdr));
	servRdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	servRdr.sin_family = AF_INET;
	servRdr.sin_port = htons(9190);

	//3.���ӷ�����
	if (connect(hSock, (SOCKADDR*)&servRdr, sizeof(servRdr)) == SOCKET_ERROR)
	{
		printf("connect error code = %d \n",GetLastError());
		return -1;
	}

	//4.���շ���˵���Ϣ ����һ���߳̽���
	hRecvThread=(HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

	//5.������Ϣ������� ����һ���̷߳�����Ϣ
	hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);

	//�ȴ��ں˶�����źŷ����仯
	WaitForSingleObject(hSendThread, INFINITE);
	WaitForSingleObject(hRecvThread, INFINITE);

	//6.�ر��׽���
	closesocket(hSock);

	WSACleanup();

	//system("pause");
	return 0;
}