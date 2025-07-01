main() extrn printf; {
	auto nums[10], i;

	nums[0] = 1;
	i = 1; while (i < 10) {
		nums[i] = nums[i-1] * 2;
		++i;
	}

	i = 0; while (i < 10) {
		printf("[%d] = %d*n", i, nums[i]);
		++i;
	}
}
