//-------------------------------------------------------------------------------------------------
//	Created:	2015-6-23   17:46
//	File Name:	Server.cpp
//	Author:		Eric(ɳӥ)
//	PS:			�������˵�����󣬴���������߼������⣬������⣬������ҡ�лл��
//  Email:		frederick.dang@gmail.com
//	Purpose:	��ɶ˿���ϰ����
//-------------------------------------------------------------------------------------------------
#include "System.h"
#include "Common.h"

#define IOCP_ASSERT(e, info) {if(!(e)) {printf(info); printf("GetLastError [%d].\n", WSAGetLastError()); fflush(stdout); assert(false);}}
#define OverLappedBufferLen 10240
#define WaitingAcceptCon 2
#define AcceptExSockAddrInLen (sizeof(SOCKADDR_IN) + 16)
#define MustPrint(s) {printf("Must >> %s\n", s); fflush(stdout);}
#define TestIPAddr "127.0.0.1"


typedef struct OverLapped
{
public:
	typedef enum OverLappedOperatorType
	{
		EOLOT_Accept = 0,
		EOLOT_Send,
		EOLOT_Recv,
	} OLOpType;

public:
	WSAOVERLAPPED	sysOverLapped;
	WSABUF			sysBuffer;
	char			dataBuffer[OverLappedBufferLen];
	OLOpType		opType;

public:
	OverLapped();
} OverLapped, *OverLappedPtr;

inline OverLapped::OverLapped()
{
	ZeroMemory(&sysOverLapped, sizeof(sysOverLapped));
	sysBuffer.buf = dataBuffer;
}

struct ThreadInfo
{
	HANDLE hIOCP;
	//LPFN_ACCEPTEX lpfAccepEx;
	SOCKET Conn;
};


DWORD ThreadProcess(LPVOID pParam)
{
	ThreadInfo* pThreadInfo = (ThreadInfo*)pParam;

	HANDLE hIOCP = pThreadInfo->hIOCP;
	SOCKET sListenConn = pThreadInfo->Conn;
	OverLapped* pOver = NULL;
	SOCKET* pConn	  = NULL;
	DWORD	dwBytes;
	DWORD	dwFlag;

	for (;;)
	{
		GetQueuedCompletionStatus(hIOCP, &dwBytes, (PULONG_PTR)&pConn, (LPOVERLAPPED*)&pOver, INFINITE);

		if (!pConn && !pOver)
			return 0;

		
		if ((dwBytes == 0 && (pOver->opType == OverLapped::OLOpType::EOLOT_Send || pOver->opType == OverLapped::OLOpType::EOLOT_Recv)) || (pOver->opType == OverLapped::OLOpType::EOLOT_Accept && WSAGetLastError() == WSA_OPERATION_ABORTED))
		{
			closesocket((SOCKET)pOver->sysBuffer.len);
			delete pOver;
		}
		else
		{
			switch (pOver->opType)
			{
				case OverLapped::OLOpType::EOLOT_Accept:
				{
					SOCKET sAcceptConn = (SOCKET)pOver->sysBuffer.len;
					int iLocalAddr, iRemoteAddr, iError;
					LPSOCKADDR pLocalAddr;
					sockaddr_in* pRemoteAddr = NULL;
					GetAcceptExSockaddrs(pOver->sysBuffer.buf, 0, AcceptExSockAddrInLen, AcceptExSockAddrInLen, 
										(PSOCKADDR*)&pLocalAddr, &iLocalAddr, (PSOCKADDR*)&pRemoteAddr, &iRemoteAddr);

					printf("new connect: %d.%d.%d.%d\n", pRemoteAddr->sin_addr.s_net, pRemoteAddr->sin_addr.s_host, pRemoteAddr->sin_addr.s_lh, pRemoteAddr->sin_addr.s_impno);
					
					// �������ӽ�����Socket��ϣ��ClientSocket���к�ListenSocket��ͬ�����ԣ���ClientSocket����SO_UPDATE_ACCEPT_CONTEXT
					// git snap (00e097d): WSAEFAULT ����4Ӧ��ָ��
					if (setsockopt(sAcceptConn, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&sListenConn, sizeof(sListenConn)) == SOCKET_ERROR)
					{
						Log("EOLOT_Accept [%d] setsockopt Error[%d].\n", sAcceptConn, WSAGetLastError());
						closesocket(sAcceptConn);
						delete pOver;
						break;
					}

					// IOCP�������� 
					// ����4��ָ��
					if (!CreateIoCompletionPort((HANDLE)sAcceptConn, hIOCP, (DWORD_PTR)&sAcceptConn, 0))
					{
						Log("EOLOT_Accept [%d] CreateIoCompletionPort Error [%d].\n", sAcceptConn, WSAGetLastError());
						closesocket(sAcceptConn);
						delete pOver;
						break;
					}

					delete pOver;

					OverLapped* pRecvOver = new OverLapped;
					pRecvOver->opType = OverLapped::EOLOT_Recv;
					pRecvOver->sysBuffer.len = OverLappedBufferLen;
					ZeroMemory(pRecvOver->dataBuffer, OverLappedBufferLen);
					// �ȴ���������
					// git snap(6fa835e): Error = WSAEOPNOTSUPP(10045 Operation not supported), ����5��flag����
					DWORD dwTemp[2] = {0, 0}; 
					int nResult = WSARecv(sAcceptConn, &pRecvOver->sysBuffer, 1, &dwTemp[0], &dwTemp[1], &pRecvOver->sysOverLapped, NULL);
					if (nResult == SOCKET_ERROR && ((iError = WSAGetLastError()) != ERROR_IO_PENDING))
					{
						Log("EOLOT_Accept [%d] WSARecv Error[%d].\n", sAcceptConn, iError);
						closesocket(sAcceptConn);
						delete pRecvOver;
						break;
					}
					Log("EOLOT_Accept [%d] WSARecv OK.\n", sAcceptConn);

					// ���͵�һ������
					// git snap(): Erro = 
					OverLapped* pSendOver = new OverLapped;
					pSendOver->opType = OverLapped::OLOpType::EOLOT_Send;
					ZeroMemory(pSendOver->dataBuffer, OverLappedBufferLen);
					sprintf_s(pSendOver->dataBuffer, "server new send time %d", GetTickCount());
					pSendOver->sysBuffer.len = strlen(pSendOver->dataBuffer);					
					int nResult2 = WSASend(sAcceptConn, &pSendOver->sysBuffer, 1, &dwBytes, 0, &pSendOver->sysOverLapped, 0);
					if (nResult2 == SOCKET_ERROR && ((iError = WSAGetLastError()) != ERROR_IO_PENDING))
					{
						Log("EOLOT_Accept [%d] WSASend Error[%d].\n", sAcceptConn, iError);				
						closesocket(sAcceptConn);
						delete pSendOver;
						break;
					}

					Log("EOLOT_Accept WSASend OK");
				}break; // OverLapped::OLOpType::EOLOT_Accept

				case OverLapped::OLOpType::EOLOT_Send:
				{
					delete pOver;
				}break; // OverLapped::OLOpType::EOLOT_Send
				
				case OverLapped::OLOpType::EOLOT_Recv:
				{
					char* pData = pOver->dataBuffer;
					printf("%s\n", pData);
					SOCKET sAcceptConn = *pConn;

					// �ȴ�������һ������
					ZeroMemory(pOver->dataBuffer, OverLappedBufferLen);
					DWORD dwTemp[2] = { 0, 0 };
					int nResult = WSARecv(sAcceptConn, &pOver->sysBuffer, 1, &dwTemp[0], &dwTemp[1], &pOver->sysOverLapped, 0);
					if (nResult == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
					{
						closesocket(sAcceptConn);
						delete pOver;
						break;
					}
					printf("Recv From Client [%s].\n", pOver->dataBuffer);


					// ģ�ⷢ�Ͳ�������
					OverLapped* pSendOver = new OverLapped;
					pSendOver->opType = OverLapped::OLOpType::EOLOT_Send;
					ZeroMemory(pSendOver->dataBuffer, OverLappedBufferLen);
					sprintf_s(pSendOver->dataBuffer, "server new send time %d", GetTickCount());
					pSendOver->sysBuffer.len = strlen(pSendOver->dataBuffer);
					int nResult2 = WSASend(sAcceptConn, &pSendOver->sysBuffer, 1, &dwBytes, 0, &pSendOver->sysOverLapped, 0);
					if (nResult2 == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
					{
						closesocket(sAcceptConn);
						delete pOver;
						break;
					}
				}break; // OverLapped::OLOpType::EOLOT_Recv
			}
		}
	}
}


void AddWaitingAcceptConn(SOCKET sListenConn, LPFN_ACCEPTEX lpfnAcceptEx)
{
	for (int a = 0; a < WaitingAcceptCon; a++)
	{
		SOCKET sAcceptConn = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, 0, 0, WSA_FLAG_OVERLAPPED);
		//SOCKET sAcceptConn = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, 0, 0, WSA_FLAG_OVERLAPPED);		
		if (sAcceptConn == INVALID_SOCKET)
			return;
		
		Log("WSASocket new AccepteConn [%d].", sAcceptConn);
		OverLapped* pAcceptExOverLapped = new OverLapped;
		pAcceptExOverLapped->opType = OverLapped::OLOpType::EOLOT_Accept;
		pAcceptExOverLapped->sysBuffer.len = (DWORD)sAcceptConn;


		// git snap(0342d1d): ���� AcceptEx ���ش��� (WSA_IO_PENDING)
		// git snap(6234b13): ���Ӳ��Դ���(BOOL bRet = AcceptEx, ����ӡ������), ���ش���(WSAEINVAL), ��ΪsAcceptConn��AcceptEx����
		DWORD dwBytes;
		BOOL bRet = AcceptEx(sListenConn, sAcceptConn, pAcceptExOverLapped->sysBuffer.buf, 0, AcceptExSockAddrInLen, AcceptExSockAddrInLen, &dwBytes, &pAcceptExOverLapped->sysOverLapped);		
		if (!bRet && WSAGetLastError() != WSA_IO_PENDING)
		{
			printf("WSAGetLastError = [%d].\n", WSAGetLastError());
			delete pAcceptExOverLapped;
			return;
		}
	}

	MustPrint("AddWaitingAcceptConn OK.\n");
}



void Flush(SOCKET sListenConn, HANDLE hAcceptExEvent, LPFN_ACCEPTEX lpfnAcceptEx)
{
	DWORD dwResult = WaitForSingleObject(hAcceptExEvent, 0);

	if (dwResult == WAIT_FAILED)
	{
		IOCP_ASSERT(false, "WaitForSingleObject return WAIT_FAILED.\n");
	}
	else if (dwResult != WAIT_TIMEOUT)
	{
		AddWaitingAcceptConn(sListenConn, lpfnAcceptEx);
	}
}

int main()
{
	WSADATA wsData;
	IOCP_ASSERT(WSAStartup(MAKEWORD(2, 2), &wsData) == 0, "WSAStartup Failed.\n");

	HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 2);
	IOCP_ASSERT(hIOCP != NULL, "CreateIoCompletionPort Failed.\n");

	SOCKET sLinstenConn = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, 0, 0, WSA_FLAG_OVERLAPPED);
	IOCP_ASSERT(sLinstenConn != INVALID_SOCKET, "WSASocket Failed.\n");
	MustPrint("Socket Create Ok.\n");

	//int nReuseAddr = 1;
	//setsockopt(Conn, SOL_SOCKET, SO_REUSEADDR, (const char*)&nReuseAddr, sizeof(int));

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6666);
	addr.sin_addr.s_addr = inet_addr(TestIPAddr); // inet_addr("127.0.0.1"); // htonl(INADDR_ANY);

	if (bind(sLinstenConn, (PSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
		IOCP_ASSERT(false, "bind Failed.\n");
	
	// ����2��	The maximum length of the queue of pending connections.  
	//			If set to SOMAXCONN, the underlying service provider responsible for socket s will set the backlog to a maximum reasonable value. 
	//			There is no standard provision to obtain the actual backlog value
	//			�ȴ����е���󳤶ȡ�
	if (listen(sLinstenConn, SOMAXCONN) == SOCKET_ERROR)
		IOCP_ASSERT(false, "listen Failed.\n");

	MustPrint("Listen OK.\n");

	// �����������Ӻ���ɶ˿�
	if (!CreateIoCompletionPort((HANDLE)sLinstenConn, hIOCP, (DWORD_PTR)&sLinstenConn, 0))
		IOCP_ASSERT(false, "CreateIoCompletionPort Associate IOCP with Conn Failed.\n");

	MustPrint("Create IOCP OK.\n");

	// Load the AcceptEx function into memory using WSAIoctl. The WSAIoctl function is an extension of the ioctlsocket()
	// function that can use overlapped I/O. The function's 3rd through 6th parameters are input and output buffers where
	// we pass the pointer to our AcceptEx function. This is used so that we can call the AcceptEx function directly, rather
	// than refer to the Mswsock.lib library.
	GUID GuidAcceptEx = WSAID_ACCEPTEX;	 // WSAID_GETACCEPTEXSOCKADDRS	
	LPFN_ACCEPTEX lpfnAcceptEx = NULL;
	DWORD dwBytes;
	int iResult = WSAIoctl(sLinstenConn, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof (GuidAcceptEx),
		&lpfnAcceptEx, sizeof (lpfnAcceptEx),
		&dwBytes, NULL, NULL);

	// ���������߳�, �̹߳�������
	ThreadInfo tThreadInfo;
	tThreadInfo.hIOCP = hIOCP;
	tThreadInfo.Conn = sLinstenConn;
	//tThreadInfo.lpfAccepEx = lpfnAcceptEx;
	HANDLE hWorkThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProcess, &tThreadInfo, 0, 0);
	IOCP_ASSERT(hWorkThread, "CreateThread Failed.\n");
	MustPrint("CreateThread OK.\n");

	// �����¼�����AcceptEx�е�Ԥ���������ʹ����ʱ���ٴδ���
	HANDLE hAcceptExEvent = CreateEvent(0, false, false, 0);
	IOCP_ASSERT(hAcceptExEvent, "CreateEvent Failed.\n");
	iResult = WSAEventSelect(sLinstenConn, hAcceptExEvent, FD_ACCEPT);
	IOCP_ASSERT(iResult != SOCKET_ERROR, "WSAEventSelect Failed.\n");
	MustPrint("Event Select OK.\n");

	// ��ӵ�һ����Ԥ��������
	AddWaitingAcceptConn(sLinstenConn, lpfnAcceptEx);

	while (true)
	{
		Flush(sLinstenConn, hAcceptExEvent, lpfnAcceptEx);
	}

	return 1;
}