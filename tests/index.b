main() extrn printf, malloc, free; {
	printf("word size = %d*n", &0[1]);

	auto nums, n, i;
	n = 10;
	nums = malloc(8 * n);

	i = 1;
	nums[0] = 1;
	while (i < n) {
		nums[i] = nums[i-1] * 2;
		++i;
	}

	i = 0;
	while (i < n) {
		printf("2 **** %d = %lld*n", i, nums[i]);
		++i;
	}

	free(nums);
}
