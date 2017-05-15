
__kernel void addmat(__global float *A, __global float *B, __global float *C)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int offset = y * get_global_size(0) + x;
	C[offset] = A[offset] + B[offset];

}
