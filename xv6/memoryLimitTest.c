#include "types.h"
#include "stat.h"
#include "user.h"

void mem_limit(void)
{
	void *m1, *m2;
	int oneMB = 1 << 20;
	int pid, ppid, i, nMB = 0;

	printf(1, "mem limit test\n");
	ppid = getpid();
	if((pid = fork()) == 0) {
	m1 = 0;
	while((m2 = malloc(oneMB)) != 0) {
		++nMB;
		*(char**)m2 = m1;
		m1 = m2;
		if (nMB % 10 == 0) {
			printf(1, "cur mem is: %dMB\n", nMB);
		}
	}
	/*for (i = 0; i < 7359; i++)
	{
		m2 = malloc(oneMB);
		*(char**)m2 = m1;
		m1 = m2;
	}*/
	printf(1, "alloc done");
	while(m1) {
		m2 = *(char**)m1;
		free(m1);
		m1 = m2;
	}
	printf(1, "mem limit is: %dMB\n", nMB);
	printf(1, "mem limit test ok\n");
	exit();
	} else {
		wait();
	}
}

int main()
{
	mem_limit();

	return 0;
}