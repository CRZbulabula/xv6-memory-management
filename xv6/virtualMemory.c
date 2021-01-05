/*
文件名:virtualMemory.c
描述：维护虚拟页式存储数据结构的函数集合，包括进程生命周期中的初始化，复制，删除，也包括内存管理中的修改和维护
*/


#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

// 提取并移除内部存储的最后一项
struct internalMemoryEntry* getlastInternalEntry(struct proc *curProcess)
{
	struct internalMemoryEntry *curTail;
	curTail = curProcess->internalEntryTail;
	if (curTail == 0)
	{
		panic("error: The last entry of internal memory is NULL!");
	}
	curProcess->internalEntryTail = curTail->pre;
	if (curTail->pre != 0)
	{
		curTail->pre->nxt = 0;
	}
	curTail->pre = 0;
	return curTail;
}

// 更新内存表项并置为表头
void setInternalHead(struct proc *curProcess, struct internalMemoryEntry* newHead, char* newVirtualAddress)
{
	newHead->nxt = curProcess->internalEntryHead;
	if (curProcess->internalEntryHead != 0)
	{
		curProcess->internalEntryHead->pre = newHead;
	}
	curProcess->internalEntryHead = newHead;
	newHead->virtualAddress = newVirtualAddress;
}

// 移除一个外存表项
void deleteExternalEntry(struct proc *curProcess, char* delVirtualAddress)
{
	int entryCnt = 0, pageCnt = 0;
	struct externalMemoryTable *curPage;

	//在原来swaptable找
	curPage = curProcess->externalListHead;
	while (curPage != 0)
	{
		for (entryCnt = 0; entryCnt < EXTERNAL_TABLE_ENTRY_NUM; entryCnt++)
		{
			if (curPage->entryList[entryCnt].virtualAddress == delVirtualAddress)
			{
				curPage->entryList[entryCnt].virtualAddress = SLOT_USABLE;
				return;
			}
		}
		curPage = curPage->nxt;
		pageCnt++;
	}
	panic("delete error: cannot find external entry!");
}

// 移除一个内存表项
void deleteInternalEntry(struct proc *curProcess, char* delVirtualAddress)
{
	if (curProcess->internalEntryHead->virtualAddress == delVirtualAddress)
	{
		curProcess->internalEntryHead->virtualAddress = SLOT_USABLE;
		if (curProcess->internalEntryHead->nxt != 0)
		{
			curProcess->internalEntryHead->pre = 0;
		}
		else
		{
			curProcess->internalEntryTail = 0;
		}
		
		struct internalMemoryEntry *newHead = curProcess->internalEntryHead->nxt;
		curProcess->internalEntryHead->nxt = 0;
		curProcess->internalEntryHead = newHead;
		curProcess->internalEntryCnt--;
	}
	else
	{
		int entryCnt = 0, pageCnt = 0;
		struct internalMemoryTable *curPage;

		curPage = curProcess->internalTableHead;
		while (curPage != 0)
		{
			for (entryCnt = 0; entryCnt < INTERNAL_TABLE_ENTRY_NUM; entryCnt++)
			{
				if (curPage->entryList[entryCnt].virtualAddress == delVirtualAddress)
				{
					struct internalMemoryEntry *curEntry = &(curPage->entryList[entryCnt]);
					curEntry->pre->nxt = curEntry->nxt;
					if (curEntry->nxt != 0)
					{
						curEntry->nxt->pre = curEntry->pre;
					}
					else
					{
						curProcess->internalEntryTail = curEntry->pre;
					}
					curEntry->virtualAddress = SLOT_USABLE;
					curEntry->pre = 0;
					curEntry->nxt = 0;
					curProcess->internalEntryCnt--;
					cprintf("proc: %d delete %d\n", curProcess->pid, delVirtualAddress);
					return;
				}
			}
			curPage = curPage->nxt;
			pageCnt++;
		}
		cprintf("target: %d cnt: %d\n", delVirtualAddress, curProcess->internalEntryCnt);
		panic("delete error: cannot find internal entry!");
	}
}

// 清理一页外存表
void clearExternalTable(struct externalMemoryTable *curTable, uint clearLink)
{
	int i;
	for (i = 0; i < EXTERNAL_TABLE_ENTRY_NUM; i++)
	{
		curTable->entryList[i].virtualAddress = SLOT_USABLE;
	}
	if (clearLink)
	{
		curTable->pre = 0;
		curTable->nxt = 0;
	}
}

// 清理一个进程的外存表
void clearExternalList(struct proc *curProcess)
{
  	struct externalMemoryTable *curPage;
  	curPage = curProcess->externalListHead;
	while (curPage != 0)
  	{
		clearExternalTable(curPage, 0);
		curPage = curPage->nxt;
  	}
}

// 生成一页新的外存表
struct externalMemoryTable* allocExternalTable()
{
	struct externalMemoryTable *newTable;
	if ((newTable = (struct externalMemoryTable *)kalloc()) == 0)
	{
		return 0;
	}
	clearExternalTable(newTable, 1);
	return newTable;
}

// 分配新的外存表
int growExternalTable(struct proc *curProcess)
{
  	struct externalMemoryTable **head, **tail;
  	head = &(curProcess->externalListHead);
  	tail = &(curProcess->externalListTail);
  	if (*head == 0)
  	{
		if ((*head = allocExternalTable()) == 0)
		{
			return -1;
		}
		*tail = *head;
  	}
  	else
  	{
		struct externalMemoryTable *tmp = *tail;
		if ((*tail = allocExternalTable()) == 0)
		{
			return -1;
		}
		tmp->pre = *tail;
		(*tail)->nxt = tmp;
		curProcess->externalListTail = *tail;
  	}
	cprintf("head: %p tail: %p\n", curProcess->externalListHead, curProcess->externalListTail);
  	return 0;
}

// 在外部存储中找到对应位置
struct externalMemoryPlace GetAddressInSwapTable(struct proc *curProcess, char* virtualAddress)
{
	int entryCnt = 0, pageCnt = 0;
	struct externalMemoryTable *curPage;
	struct externalMemoryPlace retPlace;

	curPage = curProcess->externalListHead;
	while (curPage != 0)
	{
		for (entryCnt = 0; entryCnt < EXTERNAL_TABLE_ENTRY_NUM; entryCnt++)
		{
			if (curPage->entryList[entryCnt].virtualAddress == virtualAddress)
			{
				retPlace.Entry = &(curPage->entryList[entryCnt]);
				retPlace.Offset = pageCnt * EXTERNAL_TABLE_OFFSET + entryCnt * PGSIZE;
				return retPlace;
			}
		}
		curPage = curPage->nxt;
		pageCnt++;
	}
	panic("error: cannot find specified external place!");
}

// 在外部存储中找到一个空位
struct externalMemoryPlace getEmptyExternalPlace(struct proc *curProcess)
{
	int entryCnt = 0, pageCnt = 0;
	struct externalMemoryTable *curPage;
	struct externalMemoryPlace retPlace;

	curPage = proc->externalListHead;
	while (curPage != 0)
	{
		for (entryCnt = 0; entryCnt < EXTERNAL_TABLE_ENTRY_NUM; entryCnt++)
		{
			if (curPage->entryList[entryCnt].virtualAddress == SLOT_USABLE)
			{
				retPlace.Entry = &(curPage->entryList[entryCnt]);
				retPlace.Offset = pageCnt * EXTERNAL_TABLE_OFFSET + entryCnt * PGSIZE;
				return retPlace;
			}
		}
		curPage = curPage->nxt;
		pageCnt++;
	}

	// 新增一页外部存储并继续查找
	growExternalTable(curProcess);
	curPage = proc->externalListTail;
	while (curPage != 0)
	{
		for (entryCnt = 0; entryCnt < EXTERNAL_TABLE_ENTRY_NUM; entryCnt++)
		{
			if (curPage->entryList[entryCnt].virtualAddress == SLOT_USABLE)
			{
				retPlace.Entry = &(curPage->entryList[entryCnt]);
				retPlace.Offset = pageCnt * EXTERNAL_TABLE_OFFSET + entryCnt * PGSIZE;
				return retPlace;
			}
		}
		curPage = curPage->nxt;
		pageCnt++;
	}
	panic("error: cannot find empty external place!");
}

// 清理一页虚拟内存表
void clearInternalTable(struct internalMemoryTable *curTalbe, int clearLink)
{
	int i;
	for (i = 0; i < INTERNAL_TABLE_ENTRY_NUM; i++)
	{
		curTalbe->entryList[i].pre = 0;
		curTalbe->entryList[i].nxt = 0;
		curTalbe->entryList[i].virtualAddress = SLOT_USABLE;
	}
	if (clearLink)
	{
		curTalbe->nxt = 0;
		curTalbe->pre = 0;
	}
}

// 清理进程的虚拟内存表
void clearInternalList(struct proc *curProcess)
{
	struct internalMemoryTable *curTable = curProcess->internalTableHead;
	while (curTable != 0)
	{
		clearInternalTable(curTable, 0);
		curTable = curTable->nxt;
	}
	curProcess->internalEntryCnt = 0;
	curProcess->internalEntryHead = 0;
	curProcess->internalEntryTail = 0;
}

// 生成一页虚拟内存表
struct internalMemoryTable *allocInternalTable()
{
	struct internalMemoryTable *newTable;
	if ((newTable = (struct internalMemoryTable *)kalloc()) == 0)
	{
		return 0;
	}
	clearInternalTable(newTable, 1);
	return newTable;
}

// 初始化进程的虚拟内存表
void allocInternalList(struct proc *curProcess)
{
	int i;
	struct internalMemoryTable *curTable, *preTable;
	for (i = 0; i < INTERNAL_TABLE_LENGTH; i++)
	{
		if ((curTable = allocInternalTable()) == 0)
		{
			panic("Alloc Internal Memory Table Failure!\n");
		}
		if (i)
		{
			preTable->nxt = curTable;
			curTable->pre = preTable;
			preTable = curTable;
		}
		else
		{
			curProcess->internalTableHead = curTable;
			preTable = curTable;
		}
	}
	curProcess->internalTableTail = curTable;
}

// 复制交换表和交换文件
int copyInternalMemory(struct proc *src, struct proc *dst)
{
	//复制交换表
  	clearInternalList(dst);
  	dst->internalEntryHead = 0;
  	dst->internalEntryTail = 0;
  	dst->internalEntryCnt = src->internalEntryCnt;

  	int dstEntryCnt = 0;
  	struct internalMemoryTable *curDstPage = dst->internalTableHead;
  	struct internalMemoryEntry *curSrcEntry = src->internalEntryHead;
  	struct internalMemoryEntry *preDstEntry = 0;

  	if (curSrcEntry != 0)
  	{
		dst->internalEntryHead = &(curDstPage->entryList[dstEntryCnt]);
  	}
  	while (curSrcEntry != 0)
  	{
		if (preDstEntry != 0)
		{
	  		preDstEntry->nxt = &(curDstPage->entryList[dstEntryCnt]);
		}
		curDstPage->entryList[dstEntryCnt].pre = preDstEntry;
		preDstEntry = &(curDstPage->entryList[dstEntryCnt]);
		curDstPage->entryList[dstEntryCnt].virtualAddress = curSrcEntry->virtualAddress;
		dstEntryCnt++;
		curSrcEntry = curSrcEntry->nxt;
		if (dstEntryCnt == INTERNAL_TABLE_ENTRY_NUM)
		{
	  		curDstPage = curDstPage->nxt;
	  		dstEntryCnt = 0;
		}
  	}
  	dst->internalEntryTail = preDstEntry;

	//复制交换文件
  	int i;
  	struct externalMemoryTable *SourceSwapPage, *DestinationSwapPage;
  	while (src->externalEntryCnt > dst->externalEntryCnt)
  	{
		if (growExternalTable(dst) == 0)
		{
	  		return -1;
		}
		dst->externalEntryCnt++;
  	}

  	SourceSwapPage = src->externalListHead;
  	DestinationSwapPage = dst->externalListHead;
  	while (SourceSwapPage != 0)
  	{
		for (i = 0; i < EXTERNAL_TABLE_ENTRY_NUM; i++)
		{
	  		DestinationSwapPage->entryList[i] = SourceSwapPage->entryList[i];
		}
		DestinationSwapPage = DestinationSwapPage->nxt;
		SourceSwapPage = SourceSwapPage->nxt;
  	}
  	return 0;
}