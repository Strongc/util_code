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
#include <map>
#include <utility>
using namespace std;

typedef void  (CALLBACK *pTimerProc)(int, void* );


class CTimer
{
public:
	CTimer();
	virtual ~CTimer();

public:

	//������ʱ��
	int StartTimer(int nIndex, pTimerProc pfnTimerProc, unsigned int nMilliSeconds, void* param);

	//ֹͣ��ʱ��
	int StopTimer();

	int KillTimer(int nIndex);

	//��ȡΨһʵ��
	//static CTimer* GetInstance();

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

	static  void* CALLBACK ThreadProc(void* pParam);
};

#endif // !defined(AFX_TIMER_H__A98C6783_0D74_4BF4_9CE9_496A49D828A0__INCLUDED_)
