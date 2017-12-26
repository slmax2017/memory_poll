#ifndef _CMEMPOOL_H__
#define _CMEMPOOL_H__


class CMemPool
{
public:
	CMemPool();
	~CMemPool();
public:
	int initPool(const unsigned long Num);
	char* allocMem(const unsigned long iSize);
	int releaseMem(const void *p);
private:
	void	*m_Pool;
	void	*m_pLargePool;
	char* allocSmall(const unsigned long iIndex);
	char* allocLarge(unsigned long iSize);
	int releaseSmall(const void *pUnit);
};
#endif