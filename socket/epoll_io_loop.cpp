#include "io_loop.h"
#include "HPR_Select.h"

CEpollIOLoop::CEpollIOLoop(void)
{
	m_eid = 0;
}

CEpollIOLoop::~CEpollIOLoop(void)
{
}

/**	@fn	void CEpollIOLoop::Start()
*	@brief 
*	@return	
*/
void CEpollIOLoop::Start()
{
	if (m_threadhandle == HPR_INVALID_THREAD)
	{
		m_waker.Start();
		m_eid = epoll_create(EPOLL_SIZE);
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
	epoll_ctl(eid, EPOLL_CTL_ADD, &ev);

	while (True)
	{
		struct epoll_event events[EPOLL_SIZE];

		HPR_INT32 nfds = epoll_wait(m_eid, events, EPOLL_SIZE, -1);
		if (nfds <= 0)
			continue;

		for (HPR_INT32 i = 0; i < nfds; i++)
		{
			HPR_SOCKET sock = events[i].data.fd;
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
					//ò��epoll���ڵ���close(sock)����Լ������
					//epoll_ctl(eid, EPOLL_CTL_DEL, &events[i]);
				}
			}
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
	epoll_ctl(m_eid, EPOLL_CTL_ADD, &ev);
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
	m_MapIOStreamBySocket.erase(piostream->GetSocket());
	//�ر�socket��ʱ��epoll���Զ��Ӽ�����ɾ����socket?
	//struct epoll_event ev;
	//ev.data.fd=piostream->GetSocket();
	//ev.events=EPOLLIN;
	//epoll_ctl(m_eid, EPOLL_CTL_DEL, &events[i]);
	m_waker.Wake();
	m_MapMutex.Unlock();
}
