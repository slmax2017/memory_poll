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

	/*小内存释放*/

}

int CMemPool::initPool(const unsigned long Num)
{
	MemPool_t *pPool = (MemPool_t *)m_Pool;

	for (int i = 0; i < BLOCKS; ++i)
	{
		MemBlock_t *pBlock	= (MemBlock_t *)malloc(sizeof(MemBlock_t));
		assert(pBlock);

		//内存块初始化
		pBlock->IdleUints = 0;
		pBlock->usedUints = 0;
		pBlock->pMemPool = pPool + i;
		pBlock->pNext = NULL;

		//内存池初始化
		pPool[i].blocks = 1;
		pPool[i].pBlock = pBlock;
		pPool[i].pFirstUnit = NULL;
		pPool[i].unitSize = POW(2, i + 2);

		//创建BLOCKS个内存单元
		for (int j = 0; j < BLOCKS; ++j)
		{
			MemUnit_t *pUnit = (MemUnit_t *)malloc(sizeof(MemUnit_t) + POW(2, i + 2));
			assert(pUnit);
			bzero(pUnit, POW(2, i + 2) + sizeof(MemUnit_t));

			//内存单元初始化
			pUnit->pData = (char *)(pUnit + 1);
			pUnit->pBlock = pBlock;
			pUnit->pNext = NULL;
			pUnit->iMark = SMALL;

			//获取当前粒度的内存池
			MemPool_t *pCurPool = pBlock->pMemPool;

			//组装内存单元链表
			if (!pCurPool->pFirstUnit) {
				pCurPool->pFirstUnit = pUnit;
			}
			else {
				MemUnit_t *pUnitPos = pCurPool->pFirstUnit;	 //定位到第一个内存单元

				while (pUnitPos->pNext)
					pUnitPos = pUnitPos->pNext;					//移动到最后一个单元

				pUnitPos->pNext = pUnit;						//链接新单元
			}

			//标记空闲内存单元
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

	//回收的内存单元
	MemUnit_t *pType = NULL;
	MemUnit_t *pRecyUnit = entry(ptr, pType, pData);
	
	pRecyUnit->pNext = NULL;
	pRecyUnit->pPrev = NULL;

	/*小内存回收方案*/
	if (pRecyUnit->iMark == SMALL){
		bzero(pRecyUnit + 1, pRecyUnit->pBlock->pMemPool->unitSize);
		return releaseSmall(pRecyUnit);
	}
		



	/* 大内存回收方案, 
	  大内存回收由内存池管理, 即m_pLargePool
	 */
	bzero(pRecyUnit + 1, (long)(pRecyUnit->pBlock));

	/*获取大内存内存池*/
	MemPool_t *pLarge = (MemPool_t *)m_pLargePool;

	/*获取第一个内存单元,用于遍历*/
	MemUnit_t *pUnit = pLarge->pFirstUnit;

	//最后一个单元
	MemUnit_t *pLast = pUnit;



	//如果没有内存单元
	if (!pUnit){
		pLarge->pFirstUnit = pRecyUnit;
		pLarge->unitSize = (long)(pRecyUnit->pBlock);
	}
	


	//已有内存单元
	else {

		while (pUnit)
		{
			int iRecv = (long)(pRecyUnit->pBlock);
			int iSize = (long)(pUnit->pBlock);

			if (iRecv <= iSize) {

				/*如果进行比较的单元在第一个位置*/
				if (pUnit == pLarge->pFirstUnit){

					//将回收的放在第一个位置
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

			/*记录链表最后一个单元*/
			if (pUnit)
				pLast = pUnit;
		}

		/*回收的单元大于所有, 需要链表到最后*/
		pLast->pNext = pRecyUnit;
		pRecyUnit->pPrev = pLast;
		pLarge->unitSize = (long)(pRecyUnit->pBlock); //更新链表中的最大元素
	}

back:
	++pLarge->blocks;
	return EXIT_SUCCESS;
}

char* CMemPool::allocSmall(const unsigned long iIndex)
{
	//获取当前粒度的内存池
	MemPool_t *pCurPool = (MemPool_t *)m_Pool + iIndex;
	
	//存在闲余内存单元, 直接取出返回
	if (pCurPool->pFirstUnit)
	{
		MemUnit_t *pUnit = pCurPool->pFirstUnit;
		pCurPool->pFirstUnit = pUnit->pNext;
		pUnit->pNext = NULL;
		--pCurPool->pBlock->IdleUints;
		++pCurPool->pBlock->usedUints;

		return pUnit->pData;
	}

	//获取当前粒度的具体值
	unsigned int allocLen = POW(2, iIndex + 2);

	/*
	 * 内存单元已用尽,移动到最后一个内存块,
	 * 然后创建新内存块,链接上新的内存块,再创
	 * 建新内存单元,组织好单元链表,最后返回
	 * 可用单元.
	 **/
	//移动到最后一个内存块
	MemBlock_t *pLastBlock = pCurPool->pBlock;
	while (pLastBlock->pNext)
		pLastBlock = pLastBlock->pNext;

	//创建新内存块
	MemBlock_t *pNewBlock = (MemBlock_t *)malloc(sizeof(MemBlock_t));
	assert(pNewBlock);

	//链接内存块
	pLastBlock->pNext = pNewBlock;
	++pCurPool->blocks;

	//内存块初始化
	pNewBlock->IdleUints = 0;
	pNewBlock->pMemPool = pCurPool;
	pNewBlock->pNext = NULL;
	pNewBlock->usedUints = 0;

	//创建BLOCKS - 1个内存单元
	for (int j = 0; j < BLOCKS - 1; ++j)
	{
		MemUnit_t *pUnit = (MemUnit_t *)malloc(sizeof(MemUnit_t) + allocLen);
		assert(pUnit);
		bzero(pUnit, allocLen + sizeof(MemUnit_t));

		//内存单元初始化
		pUnit->pData = (char *)(pUnit + 1);
		pUnit->pBlock = pNewBlock;
		pUnit->pNext = NULL;
		pUnit->iMark = SMALL;

		//组装内存单元链表
		if (!pCurPool->pFirstUnit) {
			pCurPool->pFirstUnit = pUnit;
		}else {
			MemUnit_t *pUnitPos = pCurPool->pFirstUnit;	 //定位到第一个内存单元

			while (pUnitPos->pNext)
				pUnitPos = pUnitPos->pNext;					//移动到最后一个单元

			pUnitPos->pNext = pUnit;						//链接新单元
		}

		//标记空闲内存单元
		pNewBlock->IdleUints++;
	}

	MemUnit_t *pUnit = (MemUnit_t *)malloc(sizeof(MemUnit_t) + allocLen);
	assert(pUnit);
	bzero(pUnit, allocLen + sizeof(MemUnit_t));

	//内存单元初始化
	pUnit->pData = (char *)(pUnit + 1);
	pUnit->pBlock = pNewBlock;
	pUnit->pNext = NULL;
	pUnit->iMark = SMALL;

	++pNewBlock->usedUints;

	return pUnit->pData;
}

char* CMemPool::allocLarge(unsigned long iSize)
{
	//大内存链表
	MemPool_t *pLarge = (MemPool_t *)m_pLargePool;

	if (pLarge->blocks > 0 && pLarge->unitSize > iSize)
	{
		MemUnit_t *pUnit = pLarge->pFirstUnit;
		while (pUnit)
		{
			int unitSize = (long)(pUnit->pBlock);	  //取出当前内存单元的内存大小
			if (unitSize >= iSize)
			{
				//如果是第一个内存单元
				if (pUnit == pLarge->pFirstUnit){
					pLarge->pFirstUnit = pUnit->pNext;
					pLarge->pFirstUnit ? pLarge->pFirstUnit->pPrev = NULL : 0;
					pUnit->pNext = NULL;
					pUnit->pPrev = NULL;

					//如果单元链表为空, 只有一个单元, 且被分配了
					if (!pLarge->pFirstUnit) 
						pLarge->unitSize = 0;
					
				}


				//如果是最后一个节点
				else if (!pUnit->pNext){
					MemUnit_t *pPrev = pUnit->pPrev;
					pUnit->pPrev = NULL;
					pPrev->pNext = NULL;
					pLarge->unitSize = (long)(pPrev->pBlock);
				}


				//如果是中间的元素
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

	/* 没有空闲内存单元或者所有内存单元比需要开辟的空间小
	 * 需要向操作系统申请,然后可以直接返回内存空间
	 * 不需要链接到大内存链表,但要在释放时链接进来
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

	/*小内存由内存块进行管理*/
	++pBlock->IdleUints;
	--pBlock->usedUints;

	pRecyUnit->pNext = pCurPool->pFirstUnit;
	pCurPool->pFirstUnit = pRecyUnit;

	return EXIT_SUCCESS;
}
