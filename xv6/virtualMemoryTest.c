#include "types.h"
#include "stat.h"
#include "user.h"

void mem_limit(void)
{
	void *m1, *m2;
	int oneMB = 1 << 20;
	int pid, ppid, i, nMB = 252;

	printf(1, "==================================\n");
	printf(1, "Virtual Memory test begin. Test by malloc %dMB.\n", nMB);
	ppid = getpid();
	if((pid = fork()) == 0)
	{
		m1 = 0;
		for (i = 0; i < 252; i++)
		{
			m2 = malloc(oneMB);
			printf(1, "cur memory is: %dMB\n", i + 1);
			*(char**)m2 = m1;
			m1 = m2;
		}
		printf(1, "alloc done\n");
		while(m1)
		{
			m2 = *(char**)m1;
			free(m1);
			m1 = m2;
		}
		printf(1, "Virtual Memory test finish.\n");
		printf(1, "==================================\n");
		exit();
	}
	else
	{
		wait();
	}
}

int main()
{
	mem_limit();

	return 0;
}