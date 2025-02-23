/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - INITIALIZE DYNAMIC ALLOCATOR
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; // ensure it's a multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	LIST_INIT(&freeBlocksList);

	uint32* das = (uint32*) daStart;
	uint32* dae = (uint32*) (daStart + initSizeOfAllocatedSpace - sizeof(int));

	*das = 1;
	*dae = 1;

	uint32* blkH = (uint32*) (daStart + sizeof(int));
	uint32* blkF = (uint32*) (daStart + initSizeOfAllocatedSpace - 2 * sizeof(int));

	uint32 blsz = initSizeOfAllocatedSpace - 2 * sizeof(int); // to clear the size of begin and end block.

	*blkH = blsz;
	*blkF = blsz;

	struct BlockElement* Fisrtblk = (struct BlockElement*) (daStart + 2 * sizeof(int));

	LIST_INSERT_HEAD(&freeBlocksList, Fisrtblk);
}

//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================

/*uint32 setAllocated(uint32 size) { // 0000000 1
    return size | 1;
}*/

/*uint32 clearAllocated(uint32 size) {
    return size & ~1;
}*/

/*int isAllocated(uint32 size) {
    return size & 1;
}*/

void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
	//TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("set_block_data is not implemented yet");
	//Your Code is Here...

	uint32* h = va - sizeof(uint32);
	uint32* f = va + totalSize -  2 * sizeof(uint32);

	if (isAllocated)
		totalSize=(totalSize|1);
	else
		totalSize=(totalSize&-1);

	*h = totalSize;
	*f = totalSize;
}


//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	// Ensure the size is aligned to a suitable boundary (e.g., 8 bytes)
	if (size % 2 != 0) size++; // ensure that the size is even
	if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
		size = DYN_ALLOC_MIN_BLOCK_SIZE;

	if (!is_initialized)
	{
		uint32 required_size = size + 2 * sizeof(int) /*header & footer*/ + 2 * sizeof(int) /*da begin & end*/;
		uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE) / PAGE_SIZE);
		uint32 da_break = (uint32)sbrk(0);

		if (da_start == (uint32)(-1)) return NULL; // Check sbrk success
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}
	// | |    | |     | |
	uint32 total_sz = size + 2 * sizeof(int); // Total size including header and footer
	struct BlockElement *itr;
	bool can = 0,large=0;
	// === ====
	//  a       f
	// H===F    ======
			//  va,size,0
	//      //  size|flag
			//  ,
	// Iterate over the free blocks list to find a suitable block using First Fit strategy
	LIST_FOREACH(itr, &freeBlocksList)
	{
		uint32 bsz = get_block_size(itr);

		// Check if the current block is large enough
		if (bsz >= total_sz)
		{
			can = 1;
			uint32 rem_sz = bsz - total_sz;

			// Allocate the current block and set its size and status
			// Mark block as allocated

			// If there's enough space for a new free block, split the block
			if (rem_sz >= 4 * sizeof(uint32)) // Ensure enough space for header/footer
			{
				large=1;
				set_block_data(itr, total_sz, 1);// full data of 1st block

				struct BlockElement *nxt_f_blk = (struct BlockElement *)((uint32)itr + total_sz);
				set_block_data(nxt_f_blk, rem_sz, 0); // Adjust size for new block
			    //--> LIST_ENTRY(next_free_block)prev_next_info;

				// itr   nxt_free
				// ->{}  { }        { }  {   }

				LIST_INSERT_AFTER(&freeBlocksList, itr, nxt_f_blk); // ok
			}

			if(!large) // internal freg
				set_block_data(itr, bsz, 1);

			LIST_REMOVE(&freeBlocksList, itr);

			//
			// H===F
			// =====
			return (void *)((uint32)itr); // Return address of the allocated space (adjust if necessary)
		}
	}

	// If no suitable block was found, call sbrk to request more memory
	if (!can)
		sbrk(4*sizeof(uint32));

	return NULL; // Return NULL if allocation fails
}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	// Ensure the size is aligned to a suitable boundary (e.g., 8 bytes)
	    if (size % 2 != 0) size++;
	    if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
	        size = DYN_ALLOC_MIN_BLOCK_SIZE;

	    if (!is_initialized)
	    {
	        uint32 required_size = size + 2 * sizeof(int) /*header & footer*/ + 2 * sizeof(int) /*da begin & end*/;
	        uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE) / PAGE_SIZE);
	        uint32 da_break = (uint32)sbrk(0);

	        if (da_start == (uint32)(-1)) return NULL;
	        initialize_dynamic_allocator(da_start, da_break - da_start);
	    }

	    uint32 total_sz = size + 2 * sizeof(int);

	    struct BlockElement *itr, *be_blk = NULL;
	    uint32 bste_sz = (uint32) -1;
	    bool can = 0;

	    LIST_FOREACH(itr, &freeBlocksList)
	    {
	        uint32 bsz = get_block_size(itr);

	        if (bsz >= total_sz)
	        {
	            can = 1;
	            if (bsz < bste_sz)
	            {
	                bste_sz = bsz;
	                be_blk = itr;
	            }
	        }
	    }

	    if (!can)
	    {
	        sbrk(4 * sizeof(uint32));
	        return NULL;
	    }

	    uint32 remain_sz = bste_sz - total_sz;

	    if (remain_sz >= 4 * sizeof(uint32))
	    {
	        set_block_data(be_blk, total_sz, 1);

	        struct BlockElement *nxte_blk = (struct BlockElement *)((uint32)be_blk + total_sz);
	        set_block_data(nxte_blk, remain_sz, 0);

	        LIST_INSERT_AFTER(&freeBlocksList, be_blk, nxte_blk);
	        LIST_REMOVE(&freeBlocksList, be_blk);
	    }
	    else
	    {
	        set_block_data(be_blk, bste_sz, 1);
	        LIST_REMOVE(&freeBlocksList, be_blk);
	    }

	    return (void *)((uint32)be_blk);

}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//==== = ============================================
void free_block(void *va)
{
	//TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
	if (va == NULL || is_free_block(va))
	        return;

	    uint32 *b_h = (uint32 *)va - 1;
	    uint32 bl_sz = get_block_size(va);
	    uint32 curSize = bl_sz;
	    uint32 *b_f = (uint32 *)((uint32)va + bl_sz - 2 * sizeof(uint32));

	    uint32 *nxte_h = (uint32 *)((uint32)va + bl_sz);
	    uint32 *prev_f = (uint32 *)((uint32)va - sizeof(uint32));

	    //////////// NEXT = FREE ////////////
	    if (is_free_block(nxte_h) && !is_free_block(prev_f))
	    {
	        uint32 nxte_bl_sz = get_block_size(nxte_h);
	        bl_sz += nxte_bl_sz;

	        struct BlockElement *curB = (struct BlockElement *)nxte_h;
	        struct BlockElement *next = LIST_NEXT(curB);
	        LIST_REMOVE(&freeBlocksList, curB);


	        set_block_data(va, bl_sz, 0);

	        struct BlockElement *frBl= (struct BlockElement *)(va);
	        if (next != NULL)
	            LIST_INSERT_BEFORE(&freeBlocksList, next, frBl);
	        else
	            LIST_INSERT_HEAD(&freeBlocksList, frBl);

	    }
	    //////////// PREV = FREE ////////////
	    else if (is_free_block(prev_f) && !is_free_block(nxte_h))
	    {
	        uint32 p_sz = get_block_size(prev_f);
	        bl_sz += p_sz;

	        b_h = (uint32 *)((uint32)va - p_sz);
	        struct BlockElement * nt=LIST_NEXT((struct BlockElement *)b_h);
	        LIST_REMOVE(&freeBlocksList, (struct BlockElement *)b_h);

	        set_block_data(b_h, bl_sz, 0);
	        if(nt!=NULL)
	        	LIST_INSERT_BEFORE(&freeBlocksList, nt,(struct BlockElement *)b_h);
	        else
	        	LIST_INSERT_TAIL(&freeBlocksList,(struct BlockElement *)b_h);


	    }
	    //////////// NEXT & PREV != FREE ////////////
	    else if (!is_free_block(nxte_h) && !is_free_block(prev_f))
	    {
	        set_block_data(va, curSize, 0);
	        LIST_INSERT_TAIL(&freeBlocksList, (struct BlockElement *)va);
	    }
	    //////////// NEXT & PREV = FREE ////////////
	    else
	    {
	        uint32 prev_bl__size = get_block_size(prev_f);
	        uint32 next_bl__size = get_block_size(nxte_h);
	        bl_sz += prev_bl__size + next_bl__size;

	        struct BlockElement *curBlock = (struct BlockElement *)nxte_h;
	        struct BlockElement *next = LIST_NEXT(curBlock);

	        LIST_REMOVE(&freeBlocksList, curBlock);

	        uint32 *prev_h = (uint32 *)((uint32)va - prev_bl__size);

	        LIST_REMOVE(&freeBlocksList, (struct BlockElement *)prev_h);


	        set_block_data(prev_h, bl_sz, 0);

	        if(next!=NULL)
	        	LIST_INSERT_BEFORE(&freeBlocksList, next,(struct BlockElement *)prev_h);
	        else
	        	LIST_INSERT_TAIL(&freeBlocksList,(struct BlockElement *)prev_h);
	    }

}
//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	    if (va!=NULL&&new_size == 0) {
	        free_block(va);
	        return NULL;
	    }

	    if (va == NULL&&new_size) {
	        return alloc_block_FF(new_size);
	    }

	    if(va==NULL&&new_size == 0){
	    	return NULL;
	    }


	    uint32 o_se = get_block_size(va);
	    new_size =new_size + (2 * sizeof(uint32));


	    //in place will reallocate and check if the reminig size enough to spilt or not
	    //if enough then spilt two allocatd block and free block else treatment as it internal fragmentation
	    if (new_size <= o_se)
	    {
	        uint32 re_se = o_se - new_size;
	        if(re_se==0)
	        {
	            set_block_data(va, new_size, 1);
	        	return va;
	        }



	        if (re_se >= 4 * sizeof(uint32))
	        {
	            set_block_data(va, new_size, 1);

	            struct BlockElement* new_free_block =(struct BlockElement*)((void*)va + new_size);
	            set_block_data(new_free_block, re_se, 0);

	            LIST_INSERT_TAIL(&freeBlocksList, new_free_block);
	        }

	        if(re_se < 4 * sizeof(uint32))
	        	set_block_data(va, o_se, 1);

	        return va;
	    }

	    uint32* nex_r = (uint32*)((uint32)va + o_se);

		while (is_free_block(nex_r))
		{
			uint32 nex_ze = get_block_size(nex_r);
			uint32 combined_size = o_se + nex_ze;

			if (combined_size >= new_size)
			{
				uint32 re_sel = combined_size - new_size;

				if (re_sel >= 4 * sizeof(uint32))
				{
					set_block_data(va, new_size, 1);
					struct BlockElement* new_free_block = (struct BlockElement*)((uint32)va + new_size);
					set_block_data(new_free_block, re_sel, 0);

					struct BlockElement* next_header_2 = LIST_NEXT((struct BlockElement*)nex_r);
					LIST_REMOVE(&freeBlocksList, (struct BlockElement*)nex_r);

					if (next_header_2 != NULL)
						LIST_INSERT_BEFORE(&freeBlocksList, next_header_2, new_free_block);
					else
						LIST_INSERT_TAIL(&freeBlocksList, new_free_block);

				}
				else
				{
					LIST_REMOVE(&freeBlocksList, (struct BlockElement*)nex_r);
					set_block_data(va, combined_size, 1);
				}

				return va;
			}

			o_se = combined_size;
			nex_r = (uint32*)((uint32)nex_r + nex_ze);
		}


	    free_block(va);
	    return alloc_block_FF(new_size);
}



/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
