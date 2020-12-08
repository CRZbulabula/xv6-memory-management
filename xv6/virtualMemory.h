/*
virtualMemory.h
虚拟页式存储数据结构和常量
*/

// 虚拟内存页表：每 340 个 entry 为一组，一个页表占用 4088 bytes（约一页）内存
// 每个 proc 初始化时分配 12 个页表，最大可占用 16MB 内存
#define INTERNAL_TABLE_ENTRY_NUM 340
#define INTERNAL_TABLE_LENGTH 12
#define INTERNAL_TABLE_TOTAL_ENTRYS (INTERNAL_TABLE_ENTRY_NUM * INTERNAL_TABLE_LENGTH)

// 外存页表：1022 个 entry， 一个页表 4096 bytes（一页） 内存
// 外存页表是动态维护的
#define EXTERNAL_TABLE_ENTRY_NUM 1022
#define EXTERNAL_TABLE_OFFSET (EXTERNAL_TABLE_ENTRY_NUM * PGSIZE)

// 外存页表的 entry 对应外存文件
// 一个外存文件最大 65536 bytes，相当于 16 个外存 entry，最多 6 个文件
#define EXTERNAL_FILE_SIZE 65536
#define EXTERNAL_FILE_MAX_NUM 6
#define SWAP_BUFFER_SIZE (PGSIZE / 4) 

// 数据结构定义

// 虚拟内存表项
// 每个entry储存为当前proc分配的一页虚拟内存
// 前一页及后一页的虚拟地址
struct internalMemoryEntry
{
	char *virtualAddress;
	struct internalMemoryEntry *pre;
	struct internalMemoryEntry *nxt;
};

// 虚拟内存页表
// 每个页表存储340个虚拟内存页
// 记录前一个及后一个内存页表地址
struct internalMemoryTable
{
	struct internalMemoryTable *pre;
	struct internalMemoryTable *nxt;
	struct internalMemoryEntry entryList[INTERNAL_TABLE_ENTRY_NUM];
};

// 外存表项
// 每个entry储存为当前proc分配的一页虚拟内存
struct externalMemoryEntry
{
	char *virtualAddress;
};

// 外存页表
// 每个页表存储1022个虚拟内存也
// 记录前一个及后一个外存页表地址
struct externalMemoryTable
{
	struct externalMemoryTable *pre;
	struct externalMemoryTable *nxt;
	struct externalMemoryEntry entryList[EXTERNAL_TABLE_ENTRY_NUM];
};

struct externalMemoryPlace
{
	struct externalMemoryEntry* Place;
	int Offset;
};
