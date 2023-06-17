//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// static pthread_mutex_t memphy_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
   // pthread_mutex_lock(&memphy_lock);
   
   int numstep = 0;

   mp->cursor = 0;
   while(numstep < offset && numstep < mp->maxsz){
     /* Traverse sequentially */
     mp->cursor = (mp->cursor + 1) % mp->maxsz;
     numstep++;
   }

   // pthread_mutex_unlock(&memphy_lock);

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   // pthread_mutex_lock(&memphy_lock);

   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE) mp->storage[addr];

   // pthread_mutex_unlock(&memphy_lock);

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{  
   // pthread_mutex_lock(&memphy_lock);

   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      *value = mp->storage[addr];
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);

   // pthread_mutex_unlock(&memphy_lock);

   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct * mp, int addr, BYTE value)
{
   // pthread_mutex_lock(&memphy_lock);

   if (mp == NULL)
     return -1;

   if (!mp->rdmflg)
     return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   // pthread_mutex_unlock(&memphy_lock);

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{  
   // pthread_mutex_lock(&memphy_lock);

   if (mp == NULL)
     return -1;

   if (mp->rdmflg)
      mp->storage[addr] = data;
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);

   // pthread_mutex_unlock(&memphy_lock);

   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
    /* This setting come with fixed constant PAGESZ */
    int numfp = mp->maxsz / pagesz;
    struct framephy_struct *newfst, *fst;
    int iter = 0;

    if (numfp <= 0)
      return -1;

    /* Init head of free framephy list */ 
    fst = malloc(sizeof(struct framephy_struct));
    fst->fpn = iter;
    mp->free_fp_list = fst;

    /* We have list with first element, fill in the rest num-1 element member*/
    for (iter = 1; iter < numfp ; iter++)
    {
       newfst =  malloc(sizeof(struct framephy_struct));
       newfst->fpn = iter;
       newfst->fp_next = NULL;
       fst->fp_next = newfst;
       fst = newfst;
    }

    return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
   // pthread_mutex_lock(&memphy_lock);
   struct framephy_struct *fp = mp->free_fp_list;

   if (fp == NULL)
     return -1;

   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;

   /* MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   free(fp);
   // pthread_mutex_unlock(&memphy_lock);
   return 0;
}

int MEMPHY_dump(struct memphy_struct * mp)
{
    /*TODO dump memphy contnt mp->storage 
     *     for tracing the memory content
     */
    printf("================MEMORY CONTENT===============\n");
    printf("Address:    Content \n");
    for (int i = 0; i < mp->maxsz; i++)
      if (mp->storage[i]) printf("0x%08x: %08x \n", i, mp->storage[i]);

    return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{  
   // pthread_mutex_lock(&memphy_lock);

   struct framephy_struct *fp = mp->free_fp_list;
   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;
   mp->free_fp_list = newnode;

   // pthread_mutex_unlock(&memphy_lock);

   return 0;
}


/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   MEMPHY_format(mp,PAGING_PAGESZ);

   mp->rdmflg = (randomflg != 0)?1:0;

   if (!mp->rdmflg )   /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;

   return 0;
}

/* Some function for use_frame_list using for find victim page on global */

int MEMPHY_remmove_usedfp_by_num(struct memphy_struct *mphy, int fpn){
   struct framephy_struct *cur_fp = mphy->used_fp_list;
   struct framephy_struct *pre_fp = NULL;

   while (cur_fp != NULL) {
      if(cur_fp->fpn == fpn) {   
         if(pre_fp == NULL) {
            mphy->used_fp_list = cur_fp->fp_next;
         } else {
            pre_fp->fp_next = cur_fp->fp_next;  
         }
         cur_fp = NULL;
         return 0;
      } else {
         pre_fp = cur_fp;
         cur_fp = cur_fp->fp_next;
      }
   }

   return -1;
}

struct framephy_struct* MEMPHY_get_usedfp(struct memphy_struct* mphy) {
   struct framephy_struct *cur_fp = mphy->used_fp_list;

   if(cur_fp == NULL)  return NULL;

   mphy->used_fp_list = cur_fp->fp_next;

   return cur_fp;
}

int MEMPHY_put_usedfp(struct memphy_struct *mphy, int fpn, struct mm_struct *owner) {
   struct framephy_struct *newNode = malloc(sizeof(struct framephy_struct));

   if (newNode == NULL)
      return -1; 
 
   newNode->fpn = fpn;
   newNode->owner = owner;
   newNode->fp_next = NULL;

   if (mphy->used_fp_list == NULL) {
      mphy->used_fp_list = newNode;
   } else {
      struct framephy_struct *fp = mphy->used_fp_list;
      while (fp->fp_next != NULL) {
         fp = fp->fp_next;
      }
      fp->fp_next = newNode;
   }

   return 0;
}










//#endif