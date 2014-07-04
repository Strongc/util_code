#include "epoll_io_loop.h"
#include "HPR_Select.h"
#include "socket_io_define.h"
CEpollIOLoop::CEpollIOLoop(void)
{
	m_eid = 0;
	m_epollszie = EPOLL_SIZE;
}

CEpollIOLoop::~CEpollIOLoop(void)
{
}

#if defined(__linux__)
/**	@fn	void CEpollIOLoop::Start(int nEpollSize)
*	@brief 
*	@param[in] nEpollSize Ĭ�ϵ�epoll����size��Ĭ��ΪEPOLL_SIZE 
*	@return	
*/
void CEpollIOLoop::Start(int nEpollSize)
{
	if (m_threadhandle == HPR_INVALID_THREAD)
	{
		_SetEpollSize(nEpollSize);
		m_waker.Start();
		m_eid = epoll_create(nEpollSize);
		m_bCloseRequest = HPR_FALSE;
		m_threadhandle = HPR_Thread_Create(RunThread, this, 0);
	}
}

/**	@fn	void CEpollIOLoop::Stop()
*	@brief 
*	@return	
*/
void CEpollIOLoop::Stop()
{
	m_bCloseRequest = HPR_TRUE;
	m_waker.Wake();
	if (m_threadhandle != HPR_INVALID_THREAD)
	{
		HPR_Thread_Wait(m_threadhandle);
		m_threadhandle = HPR_INVALID_THREAD;
		m_MapIOStreamBySocket.clear();
	}
	m_waker.Stop();
	close(m_eid);
}

/**	@fn	void CEpollIOLoop::Run()
*	@brief 
*	@return	
*/
void CEpollIOLoop::Run()
{
	struct epoll_event ev;
	ev.data.fd=m_waker.GetWakeSocket();
	//����Ҫ������¼�����
	ev.events=EPOLLIN;
	epoll_ctl(m_eid, EPOLL_CTL_ADD, m_waker.GetWakeSocket(), &ev);

	while (HPR_TRUE)
	{
		struct epoll_event* events = new epoll_event[_GetEpollSize()];
		HPR_INT32 nfds = epoll_wait(m_eid, events, _GetEpollSize(), -1);
		if (nfds <= 0)
			continue;
		for (HPR_INT32 i = 0; i < nfds; i++)
		{
			HPR_SOCK_T sock = events[i].data.fd;
			if (sock == m_waker.GetWakeSocket())
			{
				m_waker.Recv();
			}
			else if (events[i].events & EPOLLIN)
			{
				CBaseIOStream* pIOStream = _GetHandlerBySock(sock);
				if (pIOStream != NULL)
				{
					if (pIOStream->GetSockType() == SOCK_TCP_SERVER)
					{
						pIOStream->OnAccept();
					}
					else
					{
						pIOStream->OnRecv();
					}
				}
				else
				{
					//���Ե�ʱ��������ݻ�û�������Ƕ����Ѿ�û�ˣ���Ҫ���
					//epollҲ���ڵ���close(sock)����Լ������
					epoll_ctl(m_eid, EPOLL_CTL_DEL, sock, &events[i]);
				}
			}//EPOLLIN
			else if (events[i].events & EPOLLOUT)
			{
				CBaseIOStream* pIOStream = _GetHandlerBySock(sock);
				if (pIOStream != NULL)
				{
					if (pIOStream->GetSockType() == SOCK_TCP_CLIENT && pIOStream->CheckConnect())
					{
						//���ӳɹ�
						pIOStream->OnConnect(HPR_TRUE);
					}
					pIOStream->SendBufferAsync();
				}
			}//EPOLLOUT
			else if (events[i].events & EPOLLERR)
			{
				CBaseIOStream* pIOStream = _GetHandlerBySock(sock);
				if (pIOStream->GetSockType() == SOCK_TCP_CLIENT && pIOStream->CheckConnect())
				{
					HPR_INT32 nError, nCode;
					socklen_t nLen; 
					nLen = sizeof(nError);     
					nCode = getsockopt(pIOStream->GetSocket(), SOL_SOCKET, SO_ERROR, &nError, &nLen);
					if (nCode < 0 || nError) 
					{     
						//����ʧ��
						SOCKET_IO_WARN("socket connect failed, nCode: %d, nError: %d.", nCode, nError);
						pIOStream->OnConnect(HPR_FALSE);
					}
				}
			}//EPOLLERR
		}
	}
}

/**	@fn	void CEpollIOLoop::Add_Handler(CBaseIOStream* piostream)
*	@brief 
*	@param[in] piostream 
*	@return	
*/
void CEpollIOLoop::Add_Handler( CBaseIOStream* piostream )
{
	m_MapMutex.Lock();
	m_MapIOStreamBySocket[piostream->GetSocket()] = piostream;
	struct epoll_event ev;
	ev.data.fd = piostream->GetSocket();
	ev.events = EPOLLIN;
	epoll_ctl(m_eid, EPOLL_CTL_ADD, piostream->GetSocket(), &ev);
	m_waker.Wake();
	m_MapMutex.Unlock();
}

/**	@fn	void CEpollIOLoop::Remove_Handler(CBaseIOStream* piostream)
*	@brief 
*	@param[in] piostream 
*	@return	
*/
void CEpollIOLoop::Remove_Handler( CBaseIOStream* piostream )
{
	m_MapMutex.Lock();
	//�ر�socket��ʱ��epoll���Զ��Ӽ�����ɾ����socket?
	struct epoll_event ev;
	ev.data.fd=piostream->GetSocket();
	epoll_ctl(m_eid, EPOLL_CTL_DEL, piostream->GetSocket(), &ev);
	m_MapIOStreamBySocket.erase(piostream->GetSocket());
	m_waker.Wake();
	m_MapMutex.Unlock();
}

/**	@fn	void CEpollIOLoop::Add_WriteEvent(CBaseIOStream* piostream)
*	@brief ע��д�¼����������tcp client��˵������������ж�connect�Ƿ�ɹ�������Ҫע������¼�
*	@param[in] piostream 
*	@return	
*/
void CEpollIOLoop::Add_WriteEvent( CBaseIOStream* piostream )
{
	if (NULL == piostream)
	{
		return;
	}
	m_MapMutex.Lock();
	if (m_MapIOStreamBySocket.find(piostream->GetSocket()) != m_MapIOStreamBySocket.end())
	{
		
		struct epoll_event ev;
		ev.data.fd=piostream->GetSocket();
		if (piostream->GetSockType() == SOCK_TCP_CLIENT && piostream->CheckConnect())
		{
			//�����ж��Ƿ�connect�ɹ�
			//����111(Connection refused)(������һ�������ڵ�IP)�������110(Connection timed out)
			//(������һ��IP���ڣ�PORTδ����)��˵,û�ж���EPOLLERR,
			//Ҳ�ᴥ����������¼��������������������ERR�¼�
			//���ӳɹ����ÿ�д�����жϣ���������ERR�ж�
			ev.events=EPOLLOUT | EPOLLERR;
			epoll_ctl(m_eid, EPOLL_CTL_MOD, piostream->GetSocket(), &ev);
		}
		else
		{
			//��д�¼�
			ev.events=EPOLLOUT;
			epoll_ctl(m_eid, EPOLL_CTL_MOD, piostream->GetSocket(), &ev);
		}
		m_waker.Wake();
	}
	m_MapMutex.Unlock();
	return;
}

/**	@fn	void CEpollIOLoop::Remove_WriteEvent(CBaseIOStream* piostream)
*	@brief ɾ��д�¼���ɶ��¼�
*	@param[in] piostream 
*	@return	
*/
void CEpollIOLoop::Remove_WriteEvent( CBaseIOStream* piostream )
{
	if (NULL == piostream)
	{
		return;
	}
	m_MapMutex.Lock();
	if (m_MapIOStreamBySocket.find(piostream->GetSocket()) != m_MapIOStreamBySocket.end())
	{
		struct epoll_event ev;
		ev.data.fd=piostream->GetSocket();
		ev.events=EPOLLIN;
		epoll_ctl(m_eid, EPOLL_CTL_MOD, piostream->GetSocket(), &ev);
		m_waker.Wake();
	}
	m_MapMutex.Unlock();
	return;
}

#endif
