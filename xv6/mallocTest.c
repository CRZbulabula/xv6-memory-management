#include "types.h"
#include "stat.h"
#include "user.h"

int main(){
	void *arr[30];
	printf(1, "mallocTest Starts.\n");
	arr[0] = malloc(55);
	arr[1] = malloc(82);
	arr[2] = malloc(143);
	arr[3] = malloc(17);
	arr[4] = malloc(106);
	arr[5] = malloc(189);
	arr[6] = malloc(21);
	arr[7] = malloc(150);
	arr[8] = malloc(135);
	arr[9] = malloc(44);
	free(arr[6]);
	free(arr[2]);
	arr[10] = malloc(18);
	free(arr[4]);
	arr[11] = malloc(103);
	free(arr[0]);
	free(arr[3]);
	arr[12] = malloc(78);
	arr[13] = malloc(67);
	free(arr[10]);
	free(arr[11]);
	arr[14] = malloc(100);
	free(arr[1]);
	free(arr[7]);
	arr[15] = malloc(201);
	arr[16] = malloc(99);
	free(arr[12]);
	arr[17] = malloc(33);
	arr[18] = malloc(65);
	free(arr[13]);
	free(arr[16]);
	arr[19] = malloc(112);
	arr[20] = malloc(68);
	arr[21] = malloc(174);
	free(arr[17]);
	arr[22] = malloc(213);
	free(arr[8]);
	arr[23] = malloc(265);
	arr[24] = malloc(210);
	free(arr[21]);
	free(arr[23]);
	arr[25] = malloc(189);
	arr[26] = malloc(104);
	free(arr[9]);
	free(arr[19]);
	arr[27] = malloc(98);
	free(arr[11]);
	arr[28] = malloc(38);
	arr[29] = malloc(46);
	
	printf(1, "mallocTest Has Done!\n");
	printf(1, "The unused piece is %d units!\n",total_piece());
	return 0;
}