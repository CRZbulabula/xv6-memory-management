#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
	void *arr[20];
	printf(1, "mallocTest Starts.\n");
	arr[0]=malloc(50);
	arr[1]=malloc(100);
	arr[2]=malloc(80);
	arr[3]=malloc(160);
	arr[4]=malloc(20);
	free(arr[0]);
	free(arr[3]);
	arr[5]=malloc(40);
	free(arr[2]);
	arr[6]=malloc(200);
	arr[7]=malloc(60);
	free(arr[5]);
	arr[8]=malloc(120);
	arr[9]=malloc(50);
	
	// for (int i=0; i<1; i++){
	// 	free( (void*)arr[9]);
	// }
	printf(1, "mallocTest Has Done!\n");
	printf(1, "The unused piece is %d bytes!\n",total_piece());
	return 0;
}