#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"
#include "traps.h"

extern char data[];  // defined by kernel.ld
pde_t *kpgdir;  // for use in scheduler()
struct segdesc gdt[NSEGS];

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
	struct cpu *c;

	// Map "logical" addresses to virtual addresses using identity map.
	// Cannot share a CODE descriptor for both kernel and user
	// because it would have to have DPL_USR, but the CPU forbids
	// an interrupt from CPL=0 to DPL=3.
	c = &cpus[cpunum()];
	c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
	c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
	c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
	c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);

	// Map cpu, and curproc
	c->gdt[SEG_KCPU] = SEG(STA_W, &c->cpu, 8, 0);

	lgdt(c->gdt, sizeof(c->gdt));
	loadgs(SEG_KCPU << 3);
	
	// Initialize cpu-local storage.
	cpu = c;
	proc = 0;
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
	pde_t *pde;
	pte_t *pgtab;

	pde = &pgdir[PDX(va)];
	if(*pde & PTE_P){
		pgtab = (pte_t*)p2v(PTE_ADDR(*pde));
	} else {
		if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
			return 0;
		// Make sure all those PTE_P bits are zero.
		memset(pgtab, 0, PGSIZE);
		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table 
		// entries, if necessary.
		*pde = v2p(pgtab) | PTE_P | PTE_W | PTE_U;
	}
	return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
	char *a, *last;
	pte_t *pte;
	
	a = (char*)PGROUNDDOWN((uint)va);
	last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
	for(;;){
		if((pte = walkpgdir(pgdir, a, 1)) == 0)
			return -1;
		if(*pte & PTE_P)
			panic("remap");
		*pte = pa | perm | PTE_P;
		if(a == last)
			break;
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
// 
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP, 
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
	void *virt;
	uint phys_start;
	uint phys_end;
	int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};

// Set up kernel part of a page table.
pde_t*
setupkvm(void)
{
	pde_t *pgdir;
	struct kmap *k;

	if((pgdir = (pde_t*)kalloc()) == 0)
		return 0;
	memset(pgdir, 0, PGSIZE);
	if (p2v(PHYSTOP) > (void*)DEVSPACE)
		panic("PHYSTOP too high");
	for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
		if(mappages(pgdir, k->virt, k->phys_end - k->phys_start, 
								(uint)k->phys_start, k->perm) < 0)
			return 0;
	return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void
kvmalloc(void)
{
	kpgdir = setupkvm();
	switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
	lcr3(v2p(kpgdir));   // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
	pushcli();
	cpu->gdt[SEG_TSS] = SEG16(STS_T32A, &cpu->ts, sizeof(cpu->ts)-1, 0);
	cpu->gdt[SEG_TSS].s = 0;
	cpu->ts.ss0 = SEG_KDATA << 3;
	cpu->ts.esp0 = (uint)proc->kstack + KSTACKSIZE;
	ltr(SEG_TSS << 3);
	if(p->pgdir == 0)
		panic("switchuvm: no pgdir");
	lcr3(v2p(p->pgdir));  // switch to new address space
	popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(pde_t *pgdir, char *init, uint sz)
{
	char *mem;
	
	if(sz >= PGSIZE)
		panic("inituvm: more than a page");
	mem = kalloc();
	memset(mem, 0, PGSIZE);
	mappages(pgdir, 0, PGSIZE, v2p(mem), PTE_W|PTE_U);
	memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
	uint i, pa, n;
	pte_t *pte;

	if((uint) addr % PGSIZE != 0)
		panic("loaduvm: addr must be page aligned");
	for(i = 0; i < sz; i += PGSIZE){
		if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
			panic("loaduvm: address should exist");
		pa = PTE_ADDR(*pte);
		if(sz - i < PGSIZE)
			n = sz - i;
		else
			n = PGSIZE;
		if(readi(ip, p2v(pa), offset+i, n) != n)
			return -1;
	}
	return 0;
}

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(pde_t *pgdir)
{
	uint i;

	if(pgdir == 0)
		panic("freevm: no pgdir");
	deallocuvm(pgdir, KERNBASE, 0);
	for(i = 0; i < NPDENTRIES; i++){
		if(pgdir[i] & PTE_P){
			char * v = p2v(PTE_ADDR(pgdir[i]));
			kfree(v);
		}
	}
	kfree((char*)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(pde_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if(pte == 0)
		panic("clearpteu");
	*pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
	pde_t *d;
	pte_t *pte;
	uint pa, i;
	char *mem;

	if((d = setupkvm()) == 0)
		return 0;
	// copy heap
	for(i = PGSIZE; i < sz; i += PGSIZE){
		if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
			panic("copyuvm: pte should exist");
		if(!(*pte & PTE_P))
		{
			panic("copyuvm: page not present");
		}
		// *pte &= ~PTE_U;
		pa = PTE_ADDR(*pte);
		if((mem = kalloc()) == 0)
			goto bad;
		memmove(mem, (char*)p2v(pa), PGSIZE);
		if(mappages(d, (void*)i, PGSIZE, v2p(mem), PTE_W|PTE_U) < 0)
			goto bad;
	}

	// copy stack
	for(i = KERNBASE - proc->stackSize; i<KERNBASE; i+=PGSIZE){
		if ((pte = walkpgdir(pgdir, (void*)i, 0)) == 0)
			panic("copyuvm: pte should exist");
		if (!(*pte & PTE_P))
			panic("copyuvm: page not present");
		pa = PTE_ADDR(*pte);
		if((mem = kalloc()) == 0)
			goto bad;
		memmove(mem, (char*)p2v(pa), PGSIZE);
		if (mappages(d, (void*)i, PGSIZE, v2p(mem), PTE_W|PTE_U) < 0)
			goto bad;
	}

	lcr3(V2P(pgdir));
	return d;

bad:
	freevm(d);
	lcr3(V2P(pgdir));
	return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char*
uva2ka(pde_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if((*pte & PTE_P) == 0)
		return 0;
	if((*pte & PTE_U) == 0)
		return 0;
	return (char*)p2v(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
	char *buf, *pa0;
	uint n, va0;

	buf = (char*)p;
	while(len > 0){
		va0 = (uint)PGROUNDDOWN(va);
		pa0 = uva2ka(pgdir, (char*)va0);
		if(pa0 == 0)
			return -1;
		n = PGSIZE - (va - va0);
		if(n > len)
			n = len;
		memmove(pa0 + (va - va0), buf, n);
		len -= n;
		buf += n;
		va = va0 + PGSIZE;
	}
	return 0;
}

// 当内部存储耗尽，将优先级最低的内部存储项移交至外部存储，并返回空出的内部存储位
struct internalMemoryEntry* getSwapEntry()
{
	// 获取优先级最低的内存表项（队尾）
	struct internalMemoryEntry* curTail = getlastInternalEntry(proc);
	// 获取一个空的外存表
	struct externalMemoryPlace externalPlace = getEmptyExternalPlace(proc);
	struct externalMemoryEntry* externalEntry = externalPlace.Entry;
	int fileOffset = externalPlace.Offset;

	//修改交换表和外存
	cprintf("swap page out 0x%x.\n", curTail->virtualAddress);
	externalEntry->virtualAddress = curTail->virtualAddress;
	if (writeExternalFile(proc, (char *)(PTE_ADDR(curTail->virtualAddress)), fileOffset, PGSIZE) == 0)
	{
		return 0;
	}
	proc->externalEntryCnt++;
	pte_t* curPageTable;

	//修改对应页表
	curPageTable = walkpgdir(proc->pgdir, (void *)curTail->virtualAddress, 0);
	kfree((char *)(p2v(PTE_ADDR(*curPageTable))));
	//*curPageTable = PTE_W | PTE_U | PTE_PO;
	*curPageTable ^= (PTE_P | PTE_PO);
	lcr3(V2P(proc->pgdir));

	return curTail;
}

void insertEntry(char *newVirtualAddress)
{
	int i, quickCnt = proc->internalEntryCnt + 1;
	struct internalMemoryTable *curPage = proc->internalTableHead;

	// 尝试快速分配一页
	while (quickCnt > INTERNAL_TABLE_ENTRY_NUM)
	{
		quickCnt -= INTERNAL_TABLE_ENTRY_NUM;
		curPage = curPage->nxt;
	}
	if (curPage->entryList[quickCnt - 1].virtualAddress == SLOT_USABLE)
	{
		// 快速分配成功
		struct internalMemoryEntry *freeEntry = &(curPage->entryList[quickCnt - 1]);
		if (freeEntry->virtualAddress == SLOT_USABLE)
		{
			freeEntry->virtualAddress = newVirtualAddress;
			freeEntry->nxt = proc->internalEntryHead;
			if (proc->internalEntryHead == 0)
			{
				proc->internalEntryHead = freeEntry;
				proc->internalEntryTail = freeEntry;
			}
			else
			{
				proc->internalEntryHead->pre = freeEntry;
				proc->internalEntryHead = freeEntry;
			}
			proc->internalEntryCnt++;
			return;
		}
	}

	curPage = proc->internalTableHead;
	while (curPage != 0)
	{
		for (i = 0; i < INTERNAL_TABLE_ENTRY_NUM; i++)
		{
			if (curPage->entryList[i].virtualAddress == SLOT_USABLE)
			{
				curPage->entryList[i].virtualAddress = newVirtualAddress;
				curPage->entryList[i].nxt = proc->internalEntryHead;
				if (proc->internalEntryHead == 0)
				{
					proc->internalEntryHead = &(curPage->entryList[i]);
					proc->internalEntryTail = &(curPage->entryList[i]);
				}
				else
				{
					proc->internalEntryHead->pre = &(curPage->entryList[i]);
					proc->internalEntryHead = &(curPage->entryList[i]);
				}
				proc->internalEntryCnt++;
				return;
			}
		}
		curPage = curPage->nxt;
	}
	panic("error: cannot insert entry.");
}

/*
把内存优先级最低的放到外存
*/
struct internalMemoryEntry* recordFile(void)
{
	cprintf("Put out a page.\n");
	return getSwapEntry();
}
// 把一个外存的虚拟地址和内存优先级最低地址交换
void swapPage(uint swapVirtualAddress)
{
	cprintf("swapping page in 0x%x.\n", swapVirtualAddress);
	char SwapBuffer[PGSIZE];
	pte_t *extPTE;
	struct internalMemoryEntry* curTail = getSwapEntry();

	struct externalMemoryPlace externalPlace = getAddressInExternal(proc, (char *)PTE_ADDR(swapVirtualAddress));
	struct externalMemoryEntry* externalEntry = externalPlace.Entry;
	int fileOffset = externalPlace.Offset;

	extPTE = walkpgdir(proc->pgdir, (void *)swapVirtualAddress, 0);
	if (!*extPTE)
	{
		panic("[ERROR] A record should be in pgdir!");
	}
	*extPTE ^= (PTE_P | PTE_PO);

	//内外存交换
	memset(SwapBuffer, 0, PGSIZE);
	readExternalFile(proc, SwapBuffer, fileOffset, PGSIZE);

	int i, bufferCnt = 0;
	for (i = 0; i < PGSIZE; i++)
		bufferCnt += SwapBuffer[i];
	
	memmove((void *)(p2v(PTE_ADDR(*extPTE))), (void *)SwapBuffer, PGSIZE);

	//更新entry,页表
	externalEntry->virtualAddress = curTail->virtualAddress;
	setInternalHead(proc, curTail, (char *)PTE_ADDR(swapVirtualAddress));
	lcr3(V2P(proc->pgdir));
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
	char *mem;
	uint a;
	if(newsz >= KERNBASE)
		return 0;
	if(newsz < oldsz)
		return oldsz;
	// can't alloc addr in stack range
	if (newsz > KERNBASE - proc->stackSize - PGSIZE){
		return 0;
	}

	a = PGROUNDUP(oldsz);
	for(; a < newsz; a += PGSIZE) {
		if (proc->internalEntryCnt >= INTERNAL_TABLE_TOTAL_ENTRYS) {
			struct internalMemoryEntry* lastEntry = getSwapEntry();
			setInternalHead(proc, lastEntry, (char*)a);
		}
		else {
			insertEntry((char*)a);
		}

		mem = kalloc();
		if(mem == 0) {
			cprintf("allocuvm out of memory\n");
			deallocuvm(pgdir, newsz, oldsz);
			return 0;
		}
		memset(mem, 0, PGSIZE);
		if (mappages(pgdir, (char*)a, PGSIZE, v2p(mem), PTE_W|PTE_U) < 0){
			deallocuvm(pgdir, newsz, oldsz);
			kfree(mem);
			cprintf("map < 0\n");
			return 0;
		}
	}
	return newsz;
}

int stackIncre(pde_t *pgdir)
{
	// cprintf("PageFault: Stack Increasing.\n");
	uint stackBottom = KERNBASE - proc->stackSize;
	uint heapTop = proc->sz;
	if (heapTop > stackBottom - PGSIZE) {
		return 0;
	}
	char* newVirtualPage = kalloc();
	if (newVirtualPage == 0) {
		cprintf("stackIncre newVirtualPage out of memory.\n");
		return 0;
	}
	uint newPysicalPage = V2P(newVirtualPage);
	uint newStackBottom = stackBottom - PGSIZE;
	if (mappages(pgdir, (char*)newStackBottom, PGSIZE, newPysicalPage, PTE_W|PTE_U) < 0) {
		deallocuvm(pgdir, newStackBottom, stackBottom);
		kfree(newVirtualPage);
		return 0;
	}
	if (proc->internalEntryCnt >= INTERNAL_TABLE_TOTAL_ENTRYS) {
		struct internalMemoryEntry* tail = recordFile();
		setInternalHead(proc, tail, (char*)newStackBottom);
	} else {
		insertEntry((char*)newStackBottom);
	}
	memset(newVirtualPage, 0, PGSIZE);
	proc->stackSize += PGSIZE;
	return 1;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
	pte_t *pte;
	uint a, pa;

	if(newsz >= oldsz)
		return oldsz;

	a = PGROUNDUP(newsz);
	for(; a  < oldsz; a += PGSIZE){
		pte = walkpgdir(pgdir, (char*)a, 0);
		if(!pte)
			a += (NPTENTRIES - 1) * PGSIZE;
		else if((*pte & PTE_P) != 0) {
			if (proc->pgdir == pgdir)
			{
				deleteInternalEntry(proc, (char*)a);
			}
			pa = PTE_ADDR(*pte);
			if(pa == 0)
				panic("kfree");
			char *v = p2v(pa);
			kfree(v);
			*pte = 0;
		}
		else if ((*pte & PTE_PO) && proc->pgdir == pgdir) {
			deleteExternalEntry(proc, (char*)a);
		}
	}
	return newsz;
}

// 缺页中断处理
void pageFault(uint err)
{
	uint va = rcr2();
	if (!(err & PGFLT_P))
	{
		pte_t* pte = &proc->pgdir[PDX(va)];
		if(((*pte) & PTE_P) != 0)
		{
			// 将被置换的页置换回来
			if(((uint*)PTE_ADDR(P2V(*pte)))[PTX(va)] & PTE_PO) 
			{
				swapPage(PTE_ADDR(va));
				return;
			}
		}
		if (va < PGSIZE){
			cprintf("error: Use a null pointer. Kill [%s] process. va=%x\n", proc->name, va);
			proc->killed = 1;
			return;
		}

		// stack increase
		uint stackBottom = KERNBASE - proc->stackSize;
		uint heapTop = proc->sz + PGSIZE;
		if (va >= heapTop && va < stackBottom) {
			if (stackIncre(proc->pgdir) == 0){
				cprintf("error: Stack increase failed. Kill [%s] process.", proc->name);
				proc->killed = 1;
			}
			lcr3(V2P(proc->pgdir));
			return;
		}

		char *mem = kalloc();
		if (mem == 0){
			cprintf("Lazy allocation failed: Memory out. Kill [%s] process.\n", proc->name);
			proc->killed = 1;
			return;
		}
		va = PGROUNDDOWN(va);
		memset(mem, 0, PGSIZE);

		if (mappages(proc->pgdir, (char*)va, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
			cprintf("Lazy allocation failed: Memory out(2). Kill [%s] process.\n", proc->name);
			proc->killed = 1;
			return;
		}
		return;
	}

	pte_t* pte;
	if (proc == 0){
		panic("pageFault: process should exist");
	}
	if ((va >= KERNBASE) || (pte = walkpgdir(proc->pgdir, (void*)va, 0) == 0) || !(*pte & PTE_P) || !(*pte & PTE_U)){
		//cprintf("error: Virtual Address Out of Range. va = %d, pte = %x, *pte = %x\n", va, pte, *pte);
		proc->killed = 1;
		return;  
	}

	// TODO: copy on write
	// flush page dir
	lcr3(V2P(proc->pgdir));
}