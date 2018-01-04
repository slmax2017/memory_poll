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
	//单元大小
	long unitSize;

	//内存块数量
	long blocks;

	//第一块空闲内存块
	MemUnit_t *pFirstUnit;

	//第一个内存块
	MemBlock_t *pBlock;
}MemPool_t;

typedef struct _tag_MemBlock_t
{
	//内存池宿主
	MemPool_t *pMemPool;

	//空闲单元块个数
	long IdleUints;

	//当前已被使用的单元块个数
	long usedUints;

	//下一个内存块
	MemBlock_t *pNext;
}MemBlock_t;

typedef struct _tag_MemUnit_t
{
	//标记大小内存
	unsigned long iMark;

	//内存块宿主,作为大内存时会被征用为内存大小
	MemBlock_t *pBlock;

	//上一个内存单元
	MemUnit_t *pPrev;

	//下一个内存单元
	MemUnit_t *pNext;

	//实际需要的数据
	char *pData;
}MemUnit_t;
