//-------------------------------------------------------------------------------------------------
//	Created:	2015-6-23   17:47
//	File Name:	System.h
//	Author:		Eric(ɳӥ)
//	PS:			�������˵�����󣬴���������߼������⣬������⣬������ҡ�лл��
//  Email:		frederick.dang@gmail.com
//	Purpose:	IOCP��Ҫ������һЩϵͳ�ļ�
//-------------------------------------------------------------------------------------------------

#ifdef _WIN32
#include <WinSock2.h>
#include <MSWSock.h>
#include <assert.h>
#include <windows.h>
#include <stdio.h>
#include <mstcpip.h>

//#include <winsock.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
#endif // _WIN32