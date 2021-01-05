#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
	printf(1, "===========Null Pointer Protect Test Begin=========\n");
	printf(1, "It will use null pointer, and then it should be killed.\n");
	
	int *nullpointer = 0;
	printf(1, "The value of null pointer is %x.\n", *nullpointer);
	printf(1, "It still alive! Null pointer protection test failed.\n");
	return 0;
}