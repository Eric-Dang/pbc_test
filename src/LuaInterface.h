//-------------------------------------------------------------------------------------------------
//	Created:	2015-7-29   20:14
//	File Name:	LuaInterface.h
//	Author:		Eric(ɳӥ)
//	PS:			�������˵�����󣬴���������߼������⣬������⣬������ҡ�лл��
//  Email:		frederick.dang@gmail.com
//	Purpose:	C��Luaճ�Ӳ�
//-------------------------------------------------------------------------------------------------
#include "Common.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// Lua ������ͷ�ļ�
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#ifdef __cplusplus
}
#endif // __cplusplus

// TODO: ��ش���ID:201508042055
//static NetMsg ReadMsg;
//static NetMsg WriteMsg;

typedef int(*FuncLua)(lua_State*);

class LuaInterface
{
public:
	LuaInterface();
	~LuaInterface();
public:
	// ��ʼ��lua_State
	bool Init();

	// ��ȡlua_State
	lua_State* GetLuaState() {return m_pLuaState;};
	
	// ����lua�ӿ�
	int Run();

	// lua�ӿ� ������Ϣ
	static int SendMsg(lua_State* L);

	// ��ʱ���� ����ȥ�������
	void SetMsgBuffer(NetMsg* pRead, NetMsg* pWrite);
private:
	static int	traceback(lua_State* L);
	int			CallErrorHandle(int errcode);
	// ����������Ҫ��lua lib
	void		LoadAllLuaLibs();

private:
	lua_State*	m_pLuaState;
	int			m_iFunTracebackRef;
	int			m_iLuaInitFunRef;
	int			m_iLuaErrorFunRef;
	static LuaInterface* m_pLuaInterface;

private:
	NetMsg*		m_pRead;
	NetMsg*		m_pWrite;
};