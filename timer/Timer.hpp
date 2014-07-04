// Timer.h: interface for the CTimer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TIMER_H__A98C6783_0D74_4BF4_9CE9_496A49D828A0__INCLUDED_)
#define AFX_TIMER_H__A98C6783_0D74_4BF4_9CE9_496A49D828A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "hpr/HPR_Mutex.h"
#include "hpr/HPR_Config.h"
#include "hpr/HPR_Utils.h"
#include "hpr/HPR_Thread.h"
#include <map>
#include <utility>
#include <assert.h>
#include <stdio.h>

using namespace std;

typedef void  (CALLBACK *pTimerProc)(int, void* );


class CTimer
{
public:
	CTimer()
	{
		m_iQuit = 0;
		m_pTimerProc = NULL;
		m_bThreadFinished = true;
		m_StartTimer = false;
	}
	virtual ~CTimer()
	{
		StopTimer();
	}

public:

	//������ʱ��
	int StartTimer(int nIndex, pTimerProc pfnTimerProc, unsigned int nMilliSeconds, void* param)
	{
		int iRet = -1;
		assert(pfnTimerProc != NULL);
		m_Mutex.Lock();
		if (m_StartTimer == false)
		{
			m_iQuit = 0;
			m_MapMutex.Lock();
			m_MapInterval[nIndex] = make_pair(nMilliSeconds, 0);
			m_MapParam[nIndex] = param;
			m_MapMutex.Unlock();
			m_pTimerProc = pfnTimerProc;

			//set to false before thread running
			m_bThreadFinished = false;
			int nRet = HPR_ThreadDetached_Create(ThreadProc, this, 0);
			if (nRet < 0)
			{
				//thread not run
				m_bThreadFinished = true;
			}
			else
			{
				iRet = 0;
				m_StartTimer = true;
			}
		}
		else
		{
			m_MapMutex.Lock();
			m_MapInterval[nIndex] = make_pair(nMilliSeconds, 0);
			m_MapParam[nIndex] = param;
			m_MapMutex.Unlock();
		}
		m_Mutex.Unlock();
		return iRet;
	}

	//ֹͣ��ʱ��
	int StopTimer()
	{
		m_iQuit = 1;
		while (!m_bThreadFinished)
		{
			HPR_Sleep(100);
		}
		//m_Interval = -1;
		m_MapInterval.clear();
		m_MapParam.clear();
		m_pTimerProc = NULL;
		m_StartTimer = false;
		return 0;
	}

	int KillTimer(int nIndex)
	{
		m_MapMutex.Lock();
		m_MapInterval.erase(nIndex);
		m_MapParam.erase(nIndex);
		m_MapMutex.Unlock();
		return 0;
	}

	static  void* CALLBACK ThreadProc(void* pParam)
	{
		//detach the thread
		CTimer* pInstance = reinterpret_cast<CTimer*>(pParam);
		pInstance->m_bThreadFinished = false;
		int iTime = 0;
		while (pInstance->m_iQuit == 0)
		{
			//Sleep( pInstance->m_Interval );
			HPR_Sleep(500);
			//iTime += 500;

			//����һ��MAP����в�������ʹ��m_pTimerProc()�������ٴδ���һ����ʱ����Ҳ�����������
			pInstance->m_MapMutex.Lock();
			map<int, pair<unsigned int, unsigned int > > tmp = pInstance->m_MapInterval;
			map<int, void*> paramtmp = pInstance->m_MapParam;
			pInstance->m_MapMutex.Unlock();

			map<int, pair<unsigned int, unsigned int > >::iterator iter = tmp.begin();
			while (iter != tmp.end())
			{
				iter->second.second += 500;
				//printf("\nʱ���ӡ��%d       %d\n", iter->first, iter->second.second);
				if (iter->second.second >= iter->second.first)
				{
					pInstance->m_pTimerProc(iter->first, paramtmp[iter->first]);
					//������ʱ��������ԭMAP���н���ǰʱ������
					pInstance->m_MapMutex.Lock();
					map<int, pair<unsigned int, unsigned int > >::iterator it = pInstance->m_MapInterval.find(iter->first);
					if (it != pInstance->m_MapInterval.end())
					{
						//��ǰʱ������Ϊ0
						it->second.second = 0;
					}
					pInstance->m_MapMutex.Unlock();
				}
				else
				{
					//û�д�����ʱ��������ԭMAP���н�ʱ�����
					pInstance->m_MapMutex.Lock();
					map<int, pair<unsigned int, unsigned int > >::iterator it = pInstance->m_MapInterval.find(iter->first);
					if (it != pInstance->m_MapInterval.end())
					{
						//��ǰʱ���������
						it->second.second += 500;
					}
					pInstance->m_MapMutex.Unlock();
				}
				iter++;
			}
		}
		pInstance->m_bThreadFinished = true;
		return NULL;
	}
public:
	HPR_Mutex m_Mutex;
	HPR_Mutex m_MapMutex;

	//Quit flag 1:quit; 0--not
	int m_iQuit;

	//unsigned int m_Interval;

	map<int/* nIndex */, pair<unsigned int/* time of miliseconds Ԥ�趨ʱ����ʱ�� */ , unsigned int/* time of miliseconds����ǰʱ�� */> > m_MapInterval;            //����������10����ʱ��	
	map<int/* nIndex */, void* > m_MapParam;
	bool m_StartTimer;

	//ʱ���߳�
	//HPR_HANDLE m_hTimeThread;

	//�߳��Ƿ������־, false:ʱ���̻߳�û�н���; true: ʱ���߳̽�����
	bool m_bThreadFinished;

	//��ʱ�ص�����
	pTimerProc m_pTimerProc;
};

#endif // !defined(AFX_TIMER_H__A98C6783_0D74_4BF4_9CE9_496A49D828A0__INCLUDED_)
