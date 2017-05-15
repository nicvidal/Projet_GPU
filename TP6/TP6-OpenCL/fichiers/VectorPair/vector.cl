
__kernel void vecmul(__global float *vec, __global float *res,float k)
{
	int index = get_global_id(0);
	if (index %2)
	{
		res[index] = k * vec[index];
		res[index] = k * vec[index];
		res[index] = k * vec[index];
		res[index] = k * vec[index];
		res[index] = k * vec[index];
		res[index] = k * vec[index];
		res[index] = k * vec[index];
		res[index] = k * vec[index];
		res[index] = k * vec[index];
		res[index] = k * vec[index];
	}
	else
	{
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];
		res[index] = 3.14 * vec[index];

	}

}
