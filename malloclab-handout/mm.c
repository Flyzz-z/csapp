/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define BLOCK_SIZE(size) (ALIGN(size+3*SIZE_T_SIZE+((size<SIZE_T_SIZE)*SIZE_T_SIZE)))

#define GET_SIZE(hp) ((*(size_t *)(hp))&~0x7)

#define GET_ALLOC(hp) ((*(size_t *)(hp))&0x1)

#define GET_PALLOC(hp) ((*(size_t *)(hp))&0x2)

#define GET_SIGN(hp) ((*(size_t *)(hp))&0x7)

#define P_ALLOC 1
#define P_FREE 0
#define P_PALLOC 2

#define SET_PACK(size,alloc) ((size&~0x7)|(alloc))

#define SET_HEAD(hp,head) ((*(size_t*)hp)=(size_t)(head))
   
#define GET_FOOT(hp) ((void*)((char*)(hp)+GET_SIZE(hp)-SIZE_T_SIZE))

#define SET_FOOT(fp,foot) SET_HEAD(fp,foot)

#define GET_FD(hp) ((void*)((char*)(hp)-GET_SIZE(hp-SIZE_T_SIZE)))

#define GET_BK(hp) ((void*)((char*)(hp)+GET_SIZE(hp)))

#define GET_PRE(hp) ((void*)(*(size_t*)((char*)(hp)+SIZE_T_SIZE)))

#define SET_PRE(hp,pre) (*(size_t*)((char*)(hp)+SIZE_T_SIZE) = (size_t)(pre))

#define GET_SUFF(hp) ((void*)(*(size_t*)((char*)(hp)+2*SIZE_T_SIZE)))

#define SET_SUFF(hp,suff) (*(size_t*)((char*)(hp)+2*SIZE_T_SIZE) = (size_t)(suff))

#define GET_START(data) ((void*)((char*)(data)-3*SIZE_T_SIZE))

#define GET_DATA(hp) ((void*)((char*)(hp)+3*SIZE_T_SIZE))

#define LIST_NUM 15


void* freelist[15];
void *last_blk;

void* get_list(size_t size)
{
    int start = 4;
    for(int i=start;i<start+LIST_NUM;i++)
    {
        if(!(size>>i))
            return freelist[i-start];
    }
    return freelist[LIST_NUM-1];
}   

/*
    添加空闲块
*/
int add(void *list,void *block)
{
    void *suff = GET_SUFF(list);
    SET_SUFF(list, block);
    SET_PRE(block, list);
    SET_SUFF(block, suff);
    if(suff!=NULL)
        SET_PRE(suff, block);
    return 0;
}

/*
    删除空闲块
*/
int del(void *block)
{
    void *suff = GET_SUFF(block);
    void *pre  = GET_PRE(block);
    SET_SUFF(pre, suff);
    if(suff!=NULL)
        SET_PRE(suff, pre);
    return 0;
}

//块状态变换，为下一块设置P_PALLOC标记
void state_tran(void *blk,int state)
{
    if(blk!=last_blk)
    {
        void *bk = GET_BK(blk);
        int palloc = state==P_ALLOC?P_PALLOC:0;
        size_t nhead = SET_PACK(GET_SIZE(bk), GET_ALLOC(bk)|palloc);
        SET_HEAD(bk, nhead);
        if(GET_ALLOC(bk)==P_FREE)
        {
            SET_FOOT(GET_FOOT(bk),nhead);
        }
    }
}

/*
尝试合并前后，并设置头尾和空闲，返回合并后开始地址,合并块未加入空闲链表
*/
void* coalesce(void *hp)
{
    //printf("start coa\n");
    void *fd = NULL,*bk = NULL;
    void *nhp = hp;
    int be_last = 0;
    int palloc = GET_PALLOC(hp);;

    size_t new_size = GET_SIZE(hp);

    if(hp!=last_blk)
    {
        bk = GET_BK(hp);
        if(!GET_ALLOC(bk))
        {   
            if(bk==last_blk)
                be_last = 1;
            new_size += GET_SIZE(bk);
            del(bk);
        }
    }

    if(!GET_PALLOC(hp)) 
    {
        if(hp==last_blk)
            be_last = 1;
        fd = GET_FD(hp);
        new_size += GET_SIZE(fd);
        nhp = fd;
        palloc = GET_PALLOC(nhp);
        del(fd);
    }
    SET_HEAD(nhp, SET_PACK(new_size, P_FREE|palloc));
    SET_FOOT(GET_FOOT(nhp), SET_PACK(new_size, P_FREE|palloc));
    if(be_last)
        last_blk = nhp;
    state_tran(nhp, P_FREE);
    return nhp;
}

/*
    截取空闲块使用，剩余部分加入空闲链表
*/
int intercept(void *blk,size_t b_size,size_t size)
{
    if(size>b_size)
    {
        return -1;
    }
    if(GET_ALLOC(blk))
    {
        return -1;
    }

    if(b_size==0)
    {
        b_size = GET_SIZE(blk);
    }

    size_t re_size = b_size - size;
    if(re_size<BLOCK_SIZE(0))
    {
        size = b_size;
        re_size = 0;
    }

    SET_HEAD(blk, SET_PACK(size, P_ALLOC|GET_PALLOC(blk)));
    //剩余块放回空闲链
    if(re_size>0)
    {
        void *rblk = (void*)((char*)blk+size);
        SET_HEAD(rblk, SET_PACK(re_size, P_FREE|P_PALLOC));
        SET_FOOT(GET_FOOT(rblk), SET_PACK(re_size, P_FREE|P_PALLOC));
        if(blk==last_blk)
        {
            last_blk = rblk;
        } 
        rblk = coalesce(rblk);
        void *list = get_list(GET_SIZE(rblk));
        add(list, rblk);
    } else {
        state_tran(blk, P_ALLOC);
    }
    return 0;
}

/*
返回一个空闲块，并未加入空闲链表
*/
void* expand_heap(size_t size)
{
    //printf("expand heap %ld\n",size);
    size_t new_size = size;
    void *hp = mem_sbrk(new_size);
    if(hp == (void*)-1)
    {
        return hp;

    }
    int palloc = GET_ALLOC(last_blk)<<1;
    SET_HEAD(hp, SET_PACK(new_size, P_FREE|palloc));
    SET_FOOT(GET_FOOT(hp), SET_PACK(new_size, P_FREE|palloc));
    // try merge
    last_blk = hp;
    hp = coalesce(hp);
    return hp;
}


int mm_check();
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

    void *hp;
    /*
        3个链表头
        0：0-64
        1：64-1024
        3：1024-inf
    */
    //printf("init start heapsize %ld\n",mem_heapsize());
    for(int i=0;i<LIST_NUM;i++) {
        hp = mem_sbrk(BLOCK_SIZE(0));
        if(hp == (void*)-1)
        {
            return -1;
        }
        SET_HEAD(hp, SET_PACK(BLOCK_SIZE(0), P_ALLOC|P_PALLOC));
        SET_FOOT(GET_FOOT(hp),SET_PACK(BLOCK_SIZE(0), P_ALLOC|P_PALLOC));
        SET_SUFF(hp, NULL);
        freelist[i] = hp;
    }
    last_blk = hp;
    if((hp=expand_heap(BLOCK_SIZE(mem_pagesize())))==(void*)-1)
    {
        return -1;
    }
    void *list = get_list(GET_SIZE(hp));
    add(list, hp);
    //mm_check();
    // printf("init success\n");
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // printf("mm malloc\n");
    if(!size)
        return NULL;
    size_t new_size = BLOCK_SIZE(size);
    void *list = get_list(size);
    while ((size_t)list<(size_t)(mem_heap_lo()+BLOCK_SIZE(0)*LIST_NUM)) {
        void *blk = GET_SUFF(list);
        //遍历查找符合大小空闲块
        while (blk != NULL) {
            size_t b_size;
            if((b_size=GET_SIZE(blk))<new_size)
            {
                blk = GET_SUFF(blk);
                continue;
            }
            del(blk);
            intercept(blk, b_size, new_size);
            //mm_check();
            return GET_DATA(blk);
        }
        list = (void*)(list+BLOCK_SIZE(0));
    }

    // 新申请
    void *hp = expand_heap(new_size);
    size_t b_size = GET_SIZE(hp);
    intercept(hp, b_size, new_size);
    //mm_check();
    return GET_DATA(hp);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    //todo: 检查合法性
    void *hp = GET_START(ptr);
    hp = coalesce(hp);
    void *list = get_list(GET_SIZE(hp));
    add(list, hp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(ptr==NULL)
    {
        return mm_malloc(size);
    }

    if(size==0)
    {
        mm_free(ptr);
        return NULL;
    }
    void *hp = GET_START(ptr);
    size_t new_size = BLOCK_SIZE(size);
    size_t old_size = GET_SIZE(hp);
    if(new_size<=old_size)
    {
        intercept(hp, old_size, new_size);
        return ptr;
    } else 
    {
        memmove((char*)ptr-SIZE_T_SIZE, ptr, old_size-SIZE_T_SIZE*3);
        void *nhp = coalesce(hp);
        size_t mer_size = GET_SIZE(nhp);
        if(mer_size>=new_size)
        {
            memmove(GET_DATA(nhp),(char*)ptr-SIZE_T_SIZE, old_size-SIZE_T_SIZE*3);
            intercept(nhp, mer_size, new_size);
            return GET_DATA(nhp);
        }

        void *mptr = mm_malloc(size);
        if(mptr==NULL)
            return NULL;
        memmove(mptr, (char*)ptr-SIZE_T_SIZE, old_size-SIZE_T_SIZE*3);

        //把原块块放回
        void *list = get_list(mer_size);
        add(list, nhp);

        return mptr;
    }
}

int mm_check()
{
    void* start = mem_heap_lo() + 3*BLOCK_SIZE(0);
    int pre_free = 1;
    int i=0;
    while (start<mem_heap_hi()) {
        size_t size = GET_SIZE(start);
        if(GET_ALLOC(start)==P_FREE&&pre_free==P_FREE)
        {
            printf("two free block don't coa\n");
            exit(-1); 
        }
        pre_free = GET_ALLOC(start);
        //printf("blk%d size%lu alloc %d Palloc %d\n",i++,size,GET_ALLOC(start),GET_PALLOC(start));
        start = (void*)((char*)(start)+size);
    }
    if(start-1!=mem_heap_hi())
    {
        printf("end %lx not at end of heap %lx\n",start,mem_heap_hi());
        exit(-1);   
    }
    return 0;
}














