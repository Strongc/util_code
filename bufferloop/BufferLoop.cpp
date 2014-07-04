#include "malloc.h"
#include "BufferLoop.h"
#include <stdio.h>
#include <string.h>
CBufferLoop::CBufferLoop(void)
{
	m_pbuffer = NULL;
	m_nbuffer_size = 0;
	m_nhead_pos = 0;
	m_ntail_pos = 0;
	m_nused_size = 0;
}

CBufferLoop::~CBufferLoop(void)
{
	destroy_buffer();
}

/**	@fn	bool CBufferLoop::create_buffer(int nsize)
*	@brief 
*	@param[in] nsize 
*	@return	
*/
bool CBufferLoop::create_buffer( int nsize )
{
	destroy_buffer();
	m_pbuffer = (char*)malloc(nsize);
	memset(m_pbuffer, 0, nsize);
	m_nbuffer_size = nsize;
	m_nused_size = 0;
	return true;
}

/**	@fn	void CBufferLoop::destroy_buffer()
*	@brief 
*	@return	
*/
void CBufferLoop::destroy_buffer()
{
	if (m_pbuffer != NULL)
	{
		free(m_pbuffer);
		m_pbuffer = NULL;
		m_nhead_pos = 0;
		m_ntail_pos = 0;
		m_nused_size = 0;
	}
}

/**	@fn	int CBufferLoop::get_buffer_size()
*	@brief 
*	@return	
*/
int CBufferLoop::get_buffer_size()
{
	return m_nbuffer_size;
}

/**	@fn	int CBufferLoop::get_used_size()
*	@brief 
*	@return	
*/
int CBufferLoop::get_used_size()
{
	return m_nused_size;
}

/**	@fn	int CBufferLoop::get_rest_size()
*	@brief 
*	@return	
*/
int CBufferLoop::get_rest_size()
{
	return get_buffer_size() - get_used_size();
}

/**	@fn	bool CBufferLoop::append_buffer(const char* pbuffer, int nbuffer_size)
*	@brief 
*	@param[in] pbuffer 
*	@param[in] nbuffer_size 
*	@return	
*/
bool CBufferLoop::append_buffer( const char* pbuffer, int nbuffer_size )
{
	bool bret = false;
	if (get_rest_size()  < nbuffer_size)  //m_ntail_pos == m_nhead_pos
	{
		return bret;
	}

	if (m_ntail_pos < m_nhead_pos)
	{
		memcpy(&m_pbuffer[m_ntail_pos], pbuffer, nbuffer_size);
	}
	else   //m_ntail_pos >= m_nhead_pos
	{
		//ʣ����ٸ��ֽ�
		int nrest_tail = m_nbuffer_size - m_ntail_pos;
		if (nrest_tail >= nbuffer_size)
		{
			memcpy(&m_pbuffer[m_ntail_pos], pbuffer, nbuffer_size);
		}
		else
		{
			
			//�����ν��п���
			memcpy(&m_pbuffer[m_ntail_pos], pbuffer, nrest_tail);
			memcpy(&m_pbuffer[0], &pbuffer[nrest_tail], nbuffer_size - nrest_tail);
			
		}
	}
	m_ntail_pos = (m_ntail_pos + nbuffer_size) % m_nbuffer_size;
	m_nused_size += nbuffer_size;
	return bret;
}

/**	@fn	bool CBufferLoop::get_buffer(char* pbuffer, int nbuffer_size, int* nreal_buffer_size)
*	@brief ��ȡָ�����ȵ����ݣ����һ���ջ�ȡ������
*	@param[in] pbuffer ���ݴ��ָ��
*	@param[in] nbuffer_size ��Ҫ��ȡ�����ݳ���
*	@param[out] nreal_buffer_size  ʵ�ʻ�õ����ݳ���
*	@return	
*/
bool CBufferLoop::get_buffer( char* pbuffer, int nbuffer_size, int* nreal_buffer_size )
{
	bool bret = true;
	
	int nrealsize = (nbuffer_size < get_used_size() ? nbuffer_size : get_used_size());
	if (m_nhead_pos <= m_ntail_pos)
	{
		memcpy(pbuffer, &m_pbuffer[m_nhead_pos], nrealsize);
	}
	else
	{
		int nrestsize = m_nbuffer_size - m_nhead_pos;
		//��ĩβ�ĳ��ȴ�����Ҫ�ĳ���
		if (nrealsize <= nrestsize)
		{
			memcpy(pbuffer, &m_pbuffer[m_nhead_pos], nrealsize);
		}
		else
		{
			//��Ҫ�����λ�ȡ,�Ȱѵ�ĩβ������ȫ����ȡ
			memcpy(pbuffer, &m_pbuffer[m_nhead_pos], nrestsize);
			//Ȼ�����ʼ����ȡʣ�������
			memcpy(&pbuffer[nrestsize], &m_pbuffer[0], nrealsize - nrestsize);
		}
	}
	m_nused_size -= nrealsize;
	m_nhead_pos = (m_nhead_pos + nrealsize) % m_nbuffer_size;
	*nreal_buffer_size = nrealsize;
	return bret;
}

/**	@fn	char CBufferLoop::get_buffer_tmp(char* pbuffer, int nbuffer_size, int* nreal_buffer_size)
*	@brief ��ȡָ����С�����ݣ����ǲ�����ո�����
*	@param[in] pbuffer ���ݴ��ָ��
*	@param[in] nbuffer_size ��Ҫ��ȡ�Ĵ�С
*	@param[in] nreal_buffer_size ʵ�ʻ�ȡ�Ĵ�С
*	@return	
*/
char CBufferLoop::get_buffer_tmp( char* pbuffer, int nbuffer_size, int* nreal_buffer_size )
{
	bool bret = true;

	int nrealsize = (nbuffer_size < get_used_size() ? nbuffer_size : get_used_size());
	if (m_nhead_pos <= m_ntail_pos)
	{
		memcpy(pbuffer, &m_pbuffer[m_nhead_pos], nrealsize);
	}
	else
	{
		int nrestsize = m_nbuffer_size - m_nhead_pos;
		//��ĩβ�ĳ��ȴ�����Ҫ�ĳ���
		if (nrealsize <= nrestsize)
		{
			memcpy(pbuffer, &m_pbuffer[m_nhead_pos], nrealsize);
		}
		else
		{
			//��Ҫ�����λ�ȡ,�Ȱѵ�ĩβ������ȫ����ȡ
			memcpy(pbuffer, &m_pbuffer[m_nhead_pos], nrestsize);
			//Ȼ�����ʼ����ȡʣ�������
			memcpy(&pbuffer[nrestsize], &m_pbuffer[0], nrealsize - nrestsize);
		}
	}
	*nreal_buffer_size = nrealsize;
	return bret;
}