#ifndef __TASKDO_H__
#define __TASKDO_H__

#include "hpr/HPR_ThreadPoolFlex.h"
#include "hpr/HPR_Thread.h"
#include "hpr/HPR_Event.h"
#include "hpr/HPR_Guard.h"
#include <queue>


#define MAX_THREAD_NUM 1
#define DEFAULT_QUEUE_MAX_SIZE  100000


class CBaseTask
{
	CBaseTask() {};
	virtual ~CBaseTask() {};
};

//��Ҫ�����������г�Ա���ͷ�
typedef struct st_taskinfo
{
	int nTaskType;
	//void* pTaskInfo
	CBaseTask * pTaskInfo;
    st_taskinfo* pNext;
	st_taskinfo()
	{
		nTaskType = 0;
		pTaskInfo = NULL;
		pNext = NULL;
	}
	~st_taskinfo()
	{
		if (pTaskInfo != NULL)
		{
			//����void*ָ�����A���ԣ�����delete���в�ͬ��Ч��������A�ڲ�����new����delete��ʱ�򲻻��ͷ��ڴ�
			//����A�ڲ�û��new,��delete��ʱ����ͷ��ڴ档
			//���Խ��龡���ֶ�delete pTaskInfo�Ķ��󣬲��ҽ���ָ��ת�������Ƕ����ָ���ڽ���delete
			//���˴���pTaskInfo�ĳ�CBaseTask���͵Ļ���󣬽�Task���̳иû��࣬���Ա����������
			delete pTaskInfo;
			pTaskInfo = NULL;
		}
		if (pNext != NULL)
		{
			delete pNext;
			pNext = NULL;
		}
	}
}TASKINFO;

template<typename T>
class CTaskQueue
{
public:
	CTaskQueue(HPR_VOID){}
	~CTaskQueue(HPR_VOID){}
	
	void RemoveAll()
	{
		T *pTask = NULL;
		HPR_Guard guard(&m_CriticalTask);

		while(!m_queueTask.empty())
		{
			pTask = m_queueTask.front();

			if(pTask)
			{
				delete pTask;
				pTask = NULL;
			}

			m_queueTask.pop();
		}
	}

	T* GetHead()
	{
		T *pTask = NULL;
		HPR_Guard guard(&m_CriticalTask);

		if(m_queueTask.empty())
		{
			return pTask;
		}

		pTask = m_queueTask.front();
		m_queueTask.pop();

		return pTask;
	}

	HPR_BOOL Add(T* pTask)
	{
		if (pTask == NULL)
		{
			return HPR_FALSE;
		}

		HPR_Guard guard(&m_CriticalTask);

		m_queueTask.push(pTask);

		return HPR_TRUE;
	}

	unsigned int GetSize()
	{
		HPR_Guard guard(&m_CriticalTask);
		unsigned int iSize = m_queueTask.size();
		return iSize;
	}

private:
	std::queue<T*> m_queueTask;
	HPR_Mutex m_CriticalTask;

};


class CTaskDo
{
public:
	CTaskDo(void)
	{
		for (int i=0 ;i< MAX_THREAD_NUM;i++)
		{
			m_hTaskThread[i] = HPR_INVALID_THREAD;
		}

		m_hTaskEvent = HPR_CreateEvent(HPR_FALSE);
		HPR_ResetEvent(m_hTaskEvent);
		m_nMaxSize = DEFAULT_QUEUE_MAX_SIZE;
		m_bRunning = HPR_FALSE;
	}
	virtual ~CTaskDo(void)
	{
		Task_Clear();
		Task_Stop();

		if(m_hTaskEvent)
		{
			HPR_CloseEvent(m_hTaskEvent);
			m_hTaskEvent = NULL;
		}
	}
public:

	static void* CALLBACK CommitTask(void* lpParam)
	{
		CTaskDo *pTaskCtrl = (CTaskDo *)lpParam;

		while(HPR_TRUE == pTaskCtrl->m_bRunning)
		{
			HPR_WaitForSingleObject(pTaskCtrl->m_hTaskEvent, HPR_INFINITE);
			while (HPR_TRUE)
			{
				TASKINFO *pTask = pTaskCtrl->m_TaskQueue.GetHead();
				if (pTask != NULL)
				{
					pTaskCtrl->Task_Do(pTask);
				}
				else
				{   
					//�����е���������ϣ��������¼��ȴ��´δ���
					HPR_ResetEvent(pTaskCtrl->m_hTaskEvent);
					break;
				}
			}
		}
		return NULL;
	}

	HPR_BOOL Task_Start(void)
	{
		Task_Stop();
		m_bRunning = HPR_TRUE;
		for (int i=0;i<MAX_THREAD_NUM;i++)
		{
			m_hTaskThread[i] = HPR_Thread_Create(CommitTask,this,0);
			if (HPR_INVALID_THREAD == m_hTaskThread[i])
			{
				//printf("Create CommitTask thread[%i] failed!",i);
			}
		}
		return HPR_TRUE;
	}

	HPR_BOOL Task_Stop(void)
	{
		if (m_bRunning == HPR_TRUE)
		{
			m_bRunning = HPR_FALSE;
			HPR_SetEvent(m_hTaskEvent);
		}

		for (int i=0;i<MAX_THREAD_NUM;i++)
		{
			if (m_hTaskThread[i] != HPR_INVALID_THREAD)
			{
				HPR_Thread_Wait(m_hTaskThread[i]);
				m_hTaskThread[i] = HPR_INVALID_THREAD;
			}
		}
		return HPR_TRUE;
	}

	/**	@fn	HPR_BOOL CTaskDo::Task_Add(TASKINFO* pTask)
	 *	@brief ���������У�
	 *	@param[in] pTask 
	 *	@return	��ӳɹ��򷵻�HPR_TRUE  ������г��������õ����ֵ���򷵻�HPR_FALSE
	 */
	HPR_BOOL Task_Add(TASKINFO* pTask)
	{
		if (m_TaskQueue.GetSize() >= DEFAULT_QUEUE_MAX_SIZE)
		{
			return HPR_FALSE;
		}
		m_TaskQueue.Add(pTask);
		HPR_SetEvent(m_hTaskEvent);
		return HPR_TRUE;
	}

	HPR_BOOL Task_Clear(void)
	{
		m_TaskQueue.RemoveAll();
		return HPR_TRUE;
	}

	void FreeTaskInfo(TASKINFO* OperTask)
	{
		if (NULL == OperTask)
		{
			return;
		}
		delete OperTask;
		OperTask = NULL;
	}

	unsigned int GetQueueSize() { return m_TaskQueue.GetSize(); }

	virtual HPR_BOOL Task_Do(TASKINFO* OperTask)
	{
		FreeTaskInfo(OperTask);
		return HPR_FALSE;
	}
	/**	@fn	HPR_UINT32 CTaskDo::SetMaxQueueSize(HPR_INT32 nMaxSize)
	 *	@brief ���ô������������Ŀ��Ĭ��ΪDEFAULT_QUEUE_MAX_SIZE
	 *	@param[in] nMaxSize 
	 *	@return	
	 */
	void SetMaxQueueSize(int nMaxSize) { m_nMaxSize = nMaxSize; }

	HPR_HANDLE m_hTaskEvent;
	HPR_BOOL m_bRunning;

private:
	CTaskQueue<TASKINFO> m_TaskQueue;
    int m_nMaxSize;
	HPR_HANDLE m_hTaskThread[MAX_THREAD_NUM];
};
#endif // __TASKDO_H__
