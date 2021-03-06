/**	@file epoll_io_loop.h
 *	@note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
 *	@brief CEPOLLIOLoop类的头文件。
 *
 *	@author		shiwei
 *	@date		2014/03/20
 *
 *	@note 该类使用epoll的LT模式完成网络IO的读写
 *	@note 历史记录：
 *	@note V1.0.0  创建文件
 */
#pragma once

#include "socket_io.h"
#include "HPR_Thread.h"
#include "HPR_Mutex.h"
#include "HPR_Socket.h"
#include "base_io_stream.h"
#include "io_loop.h"

#include <map>
#include <vector>

#if defined(__linux__)
#include <sys/epoll.h>  
#endif

using namespace std;

#define EPOLL_SIZE  256


class SOCKET_IO_DECLARE_CLASS CEpollIOLoop : public CIOLoop
{
public:
	CEpollIOLoop(void);
	virtual ~CEpollIOLoop(void);

#if defined(__linux__)
	virtual void Start(HPR_INT32 nEpollSize = EPOLL_SIZE);
	virtual void Stop();
	virtual void Run();

	virtual void Add_Handler( CBaseIOStream* piostream );
	virtual void Remove_Handler(CBaseIOStream* piostream);
	virtual void Add_WriteEvent(CBaseIOStream* piostream);
	virtual void Remove_WriteEvent(CBaseIOStream* piostream);
private:
	void _SetEpollSize(HPR_INT32 nSize) { m_epollszie = nSize; }
	HPR_INT32 _GetEpollSize() { return m_epollszie; }
#endif

private:
	HPR_INT32 m_eid;
	HPR_INT32 m_epollszie;

};
