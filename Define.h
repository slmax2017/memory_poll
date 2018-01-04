#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BLOCKS 10
#define BIG 1
#define SMALL 2
#define POW(x, y)  (long)pow(x, y)
#ifdef _WIN32
#define bzero(x, y) memset(x, 0, y)
#endif

#if __cplusplus < 201103
#define decltype typeof
#endif

#define offset(type, member) \
			(size_t)(&((type)0)->member)			

#define entry(ptr, type, member)  \
				({				   \
					decltype(type) t = (decltype(type))(ptr); \
					(decltype(t)) ((char *)t - offset(decltype(type), member));  \
				 })

typedef struct _tag_MemBlock_t MemBlock_t;
typedef struct _tag_MemUnit_t MemUnit_t;
typedef struct _tag_MemPool_t MemPool_t;

typedef struct _tag_MemPool_t
{
	//��Ԫ��С
	long unitSize;

	//�ڴ������
	long blocks;

	//��һ������ڴ��
	MemUnit_t *pFirstUnit;

	//��һ���ڴ��
	MemBlock_t *pBlock;
}MemPool_t;

typedef struct _tag_MemBlock_t
{
	//�ڴ������
	MemPool_t *pMemPool;

	//���е�Ԫ�����
	long IdleUints;

	//��ǰ�ѱ�ʹ�õĵ�Ԫ�����
	long usedUints;

	//��һ���ڴ��
	MemBlock_t *pNext;
}MemBlock_t;

typedef struct _tag_MemUnit_t
{
	//��Ǵ�С�ڴ�
	unsigned long iMark;

	//�ڴ������,��Ϊ���ڴ�ʱ�ᱻ����Ϊ�ڴ��С
	MemBlock_t *pBlock;

	//��һ���ڴ浥Ԫ
	MemUnit_t *pPrev;

	//��һ���ڴ浥Ԫ
	MemUnit_t *pNext;

	//ʵ����Ҫ������
	char *pData;
}MemUnit_t;
