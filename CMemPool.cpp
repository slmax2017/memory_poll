#include "CMemPool.h"
#include "Define.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>



CMemPool::CMemPool()
{
	m_Pool = malloc(sizeof(MemPool_t) * 10);
	m_pLargePool = malloc(sizeof(MemPool_t));
	assert(m_Pool);
	assert(m_pLargePool);

	bzero(m_pLargePool, sizeof(MemPool_t));
}

CMemPool::~CMemPool()
{
	MemPool_t *pPool = (MemPool_t *)m_Pool;
	MemPool_t *pLarge = (MemPool_t *)m_pLargePool;

	/*С�ڴ��ͷ�*/

}

int CMemPool::initPool(const unsigned long Num)
{
	MemPool_t *pPool = (MemPool_t *)m_Pool;

	for (int i = 0; i < BLOCKS; ++i)
	{
		MemBlock_t *pBlock	= (MemBlock_t *)malloc(sizeof(MemBlock_t));
		assert(pBlock);

		//�ڴ���ʼ��
		pBlock->IdleUints = 0;
		pBlock->usedUints = 0;
		pBlock->pMemPool = pPool + i;
		pBlock->pNext = NULL;

		//�ڴ�س�ʼ��
		pPool[i].blocks = 1;
		pPool[i].pBlock = pBlock;
		pPool[i].pFirstUnit = NULL;
		pPool[i].unitSize = POW(2, i + 2);

		//����BLOCKS���ڴ浥Ԫ
		for (int j = 0; j < BLOCKS; ++j)
		{
			MemUnit_t *pUnit = (MemUnit_t *)malloc(sizeof(MemUnit_t) + POW(2, i + 2));
			assert(pUnit);
			bzero(pUnit, POW(2, i + 2) + sizeof(MemUnit_t));

			//�ڴ浥Ԫ��ʼ��
			pUnit->pData = (char *)(pUnit + 1);
			pUnit->pBlock = pBlock;
			pUnit->pNext = NULL;
			pUnit->iMark = SMALL;

			//��ȡ��ǰ���ȵ��ڴ��
			MemPool_t *pCurPool = pBlock->pMemPool;

			//��װ�ڴ浥Ԫ����
			if (!pCurPool->pFirstUnit) {
				pCurPool->pFirstUnit = pUnit;
			}
			else {
				MemUnit_t *pUnitPos = pCurPool->pFirstUnit;	 //��λ����һ���ڴ浥Ԫ

				while (pUnitPos->pNext)
					pUnitPos = pUnitPos->pNext;					//�ƶ������һ����Ԫ

				pUnitPos->pNext = pUnit;						//�����µ�Ԫ
			}

			//��ǿ����ڴ浥Ԫ
			pBlock->IdleUints++;
		}
	}
	
	return EXIT_SUCCESS;
}

char* CMemPool::allocMem(const unsigned long iSize)
{
	if (iSize <= POW(2, BLOCKS + 1))
	{
		MemPool_t *pPool = (MemPool_t *)m_Pool;

		int iIndex = 0;
		for (; iIndex < 10; ++iIndex)
		{
			if (pPool[iIndex].unitSize >= iSize)
				return allocSmall(iIndex);
		}
	}
	
	return allocLarge(iSize);
}

int CMemPool::releaseMem(const void *p)
{
	assert(p);
	char *ptr = (char *)p;

	//���յ��ڴ浥Ԫ
	MemUnit_t *pType = NULL;
	MemUnit_t *pRecyUnit = entry(ptr, pType, pData);
	
	pRecyUnit->pNext = NULL;
	pRecyUnit->pPrev = NULL;

	/*С�ڴ���շ���*/
	if (pRecyUnit->iMark == SMALL){
		bzero(pRecyUnit + 1, pRecyUnit->pBlock->pMemPool->unitSize);
		return releaseSmall(pRecyUnit);
	}
		



	/* ���ڴ���շ���, 
	  ���ڴ�������ڴ�ع���, ��m_pLargePool
	 */
	bzero(pRecyUnit + 1, (long)(pRecyUnit->pBlock));

	/*��ȡ���ڴ��ڴ��*/
	MemPool_t *pLarge = (MemPool_t *)m_pLargePool;

	/*��ȡ��һ���ڴ浥Ԫ,���ڱ���*/
	MemUnit_t *pUnit = pLarge->pFirstUnit;

	//���һ����Ԫ
	MemUnit_t *pLast = pUnit;



	//���û���ڴ浥Ԫ
	if (!pUnit){
		pLarge->pFirstUnit = pRecyUnit;
		pLarge->unitSize = (long)(pRecyUnit->pBlock);
	}
	


	//�����ڴ浥Ԫ
	else {

		while (pUnit)
		{
			int iRecv = (long)(pRecyUnit->pBlock);
			int iSize = (long)(pUnit->pBlock);

			if (iRecv <= iSize) {

				/*������бȽϵĵ�Ԫ�ڵ�һ��λ��*/
				if (pUnit == pLarge->pFirstUnit){

					//�����յķ��ڵ�һ��λ��
					pRecyUnit->pNext = pLarge->pFirstUnit;
					pLarge->pFirstUnit = pRecyUnit;
					pUnit->pPrev = pRecyUnit;
				}else {
					
					pRecyUnit->pNext = pUnit;
					pRecyUnit->pPrev = pUnit->pPrev;
					pUnit->pPrev->pNext = pRecyUnit;
					pUnit->pPrev = pRecyUnit;
				}

				goto back;
			}

			pUnit = pUnit->pNext;

			/*��¼�������һ����Ԫ*/
			if (pUnit)
				pLast = pUnit;
		}

		/*���յĵ�Ԫ��������, ��Ҫ�������*/
		pLast->pNext = pRecyUnit;
		pRecyUnit->pPrev = pLast;
		pLarge->unitSize = (long)(pRecyUnit->pBlock); //���������е����Ԫ��
	}

back:
	++pLarge->blocks;
	return EXIT_SUCCESS;
}

char* CMemPool::allocSmall(const unsigned long iIndex)
{
	//��ȡ��ǰ���ȵ��ڴ��
	MemPool_t *pCurPool = (MemPool_t *)m_Pool + iIndex;
	
	//���������ڴ浥Ԫ, ֱ��ȡ������
	if (pCurPool->pFirstUnit)
	{
		MemUnit_t *pUnit = pCurPool->pFirstUnit;
		pCurPool->pFirstUnit = pUnit->pNext;
		pUnit->pNext = NULL;
		--pCurPool->pBlock->IdleUints;
		++pCurPool->pBlock->usedUints;

		return pUnit->pData;
	}

	//��ȡ��ǰ���ȵľ���ֵ
	unsigned int allocLen = POW(2, iIndex + 2);

	/*
	 * �ڴ浥Ԫ���þ�,�ƶ������һ���ڴ��,
	 * Ȼ�󴴽����ڴ��,�������µ��ڴ��,�ٴ�
	 * �����ڴ浥Ԫ,��֯�õ�Ԫ����,��󷵻�
	 * ���õ�Ԫ.
	 **/
	//�ƶ������һ���ڴ��
	MemBlock_t *pLastBlock = pCurPool->pBlock;
	while (pLastBlock->pNext)
		pLastBlock = pLastBlock->pNext;

	//�������ڴ��
	MemBlock_t *pNewBlock = (MemBlock_t *)malloc(sizeof(MemBlock_t));
	assert(pNewBlock);

	//�����ڴ��
	pLastBlock->pNext = pNewBlock;
	++pCurPool->blocks;

	//�ڴ���ʼ��
	pNewBlock->IdleUints = 0;
	pNewBlock->pMemPool = pCurPool;
	pNewBlock->pNext = NULL;
	pNewBlock->usedUints = 0;

	//����BLOCKS - 1���ڴ浥Ԫ
	for (int j = 0; j < BLOCKS - 1; ++j)
	{
		MemUnit_t *pUnit = (MemUnit_t *)malloc(sizeof(MemUnit_t) + allocLen);
		assert(pUnit);
		bzero(pUnit, allocLen + sizeof(MemUnit_t));

		//�ڴ浥Ԫ��ʼ��
		pUnit->pData = (char *)(pUnit + 1);
		pUnit->pBlock = pNewBlock;
		pUnit->pNext = NULL;
		pUnit->iMark = SMALL;

		//��װ�ڴ浥Ԫ����
		if (!pCurPool->pFirstUnit) {
			pCurPool->pFirstUnit = pUnit;
		}else {
			MemUnit_t *pUnitPos = pCurPool->pFirstUnit;	 //��λ����һ���ڴ浥Ԫ

			while (pUnitPos->pNext)
				pUnitPos = pUnitPos->pNext;					//�ƶ������һ����Ԫ

			pUnitPos->pNext = pUnit;						//�����µ�Ԫ
		}

		//��ǿ����ڴ浥Ԫ
		pNewBlock->IdleUints++;
	}

	MemUnit_t *pUnit = (MemUnit_t *)malloc(sizeof(MemUnit_t) + allocLen);
	assert(pUnit);
	bzero(pUnit, allocLen + sizeof(MemUnit_t));

	//�ڴ浥Ԫ��ʼ��
	pUnit->pData = (char *)(pUnit + 1);
	pUnit->pBlock = pNewBlock;
	pUnit->pNext = NULL;
	pUnit->iMark = SMALL;

	++pNewBlock->usedUints;

	return pUnit->pData;
}

char* CMemPool::allocLarge(unsigned long iSize)
{
	//���ڴ�����
	MemPool_t *pLarge = (MemPool_t *)m_pLargePool;

	if (pLarge->blocks > 0 && pLarge->unitSize > iSize)
	{
		MemUnit_t *pUnit = pLarge->pFirstUnit;
		while (pUnit)
		{
			int unitSize = (long)(pUnit->pBlock);	  //ȡ����ǰ�ڴ浥Ԫ���ڴ��С
			if (unitSize >= iSize)
			{
				//����ǵ�һ���ڴ浥Ԫ
				if (pUnit == pLarge->pFirstUnit){
					pLarge->pFirstUnit = pUnit->pNext;
					pLarge->pFirstUnit ? pLarge->pFirstUnit->pPrev = NULL : 0;
					pUnit->pNext = NULL;
					pUnit->pPrev = NULL;

					//�����Ԫ����Ϊ��, ֻ��һ����Ԫ, �ұ�������
					if (!pLarge->pFirstUnit) 
						pLarge->unitSize = 0;
					
				}


				//��������һ���ڵ�
				else if (!pUnit->pNext){
					MemUnit_t *pPrev = pUnit->pPrev;
					pUnit->pPrev = NULL;
					pPrev->pNext = NULL;
					pLarge->unitSize = (long)(pPrev->pBlock);
				}


				//������м��Ԫ��
				else {
					MemUnit_t *pPrev = pUnit->pPrev;
					pPrev->pNext = pUnit->pNext;
					pUnit->pNext->pPrev = pPrev;

					pUnit->pNext = NULL;
					pUnit->pPrev = NULL;
				}

				--pLarge->blocks;
				return pUnit->pData;
			}

			pUnit = pUnit->pNext;
		}
	}

	/* û�п����ڴ浥Ԫ���������ڴ浥Ԫ����Ҫ���ٵĿռ�С
	 * ��Ҫ�����ϵͳ����,Ȼ�����ֱ�ӷ����ڴ�ռ�
	 * ����Ҫ���ӵ����ڴ�����,��Ҫ���ͷ�ʱ���ӽ���
	*/
	int iIndex = 0;
	while (iSize >> iIndex)
		++iIndex;

	iSize = POW(2, iIndex);

	MemUnit_t *pUnit = (MemUnit_t *)malloc(sizeof(MemUnit_t) + iSize);
	assert(pUnit);
	bzero(pUnit, POW(2, iIndex) + sizeof(MemUnit_t));

	pUnit->iMark = BIG;
	pUnit->pBlock = (MemBlock_t *)iSize;
	pUnit->pData = (char *)(pUnit + 1);
	pUnit->pNext = NULL;
	pUnit->pPrev = NULL;

	return pUnit->pData;
}

int CMemPool::releaseSmall(const void *pUnit)
{
	MemUnit_t *pRecyUnit = (MemUnit_t *)pUnit;

	MemBlock_t *pBlock = pRecyUnit->pBlock;
	MemPool_t *pCurPool = pBlock->pMemPool;

	/*С�ڴ����ڴ����й���*/
	++pBlock->IdleUints;
	--pBlock->usedUints;

	pRecyUnit->pNext = pCurPool->pFirstUnit;
	pCurPool->pFirstUnit = pRecyUnit;

	return EXIT_SUCCESS;
}
