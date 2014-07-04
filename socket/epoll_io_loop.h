/**	@file epoll_io_loop.h
 *	@note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
 *	@brief CEPOLLIOLoop���ͷ�ļ���
 *
 *	@author		shiwei
 *	@date		2014/03/20
 *
 *	@note ����ʹ��epoll��LTģʽ�������IO�Ķ�д
 *	@note ��ʷ��¼��
 *	@note V1.0.0  �����ļ�
 */
#pragma once
#include "HPR_Thread.h"
#include "HPR_Mutex.h"
#include "HPR_Socket.h"
#include "base_io_stream.h"
#include "io_loop.h"
#include <sys/epoll.h>  
#include <map>
#include <vector>
using namespace std;

#define EPOLL_SIZE  256

class CEpollIOLoop : public CIOLoop
{
public:
	CEpollIOLoop(void);
	virtual ~CEpollIOLoop(void);

	virtual void Start();
	virtual void Stop();
	virtual void Run();

	virtual void Add_Handler( CBaseIOStream* piostream );
	virtual void Remove_Handler(CBaseIOStream* piostream);

private:
	HPR_INT32 m_eid;
};
