#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
	void *arr[100];
	for (int i=0; i<100; i++){
		arr[i] = malloc(1000);
	}
	for (int i=0; i<100; i++){
		free(arr[i]);
	}
	printf(1, "mallocTest Has Done!\n");
	printf(1, "The unused piece is %d bytes",total_piece());
	return 0;
}