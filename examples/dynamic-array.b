da_items     0;
da_count     1;
da_capacity  2;
da_sizeof 3;

da_init(da) da[da_items] = da[da_capacity] = da[da_count] = 0;
da_deinit(da) extrn free; free(da[da_items]);

da_append(da, val) extrn malloc, realloc; {
	if (da[da_items] == 0) {
		da[da_capacity] = 10;
		da[da_items] = malloc(&0[1] * da[da_capacity]);
	}

	++da[da_count];
	if (da[da_count] > da[da_capacity]) {
		da[da_items] = realloc(da[da_items], da[da_capacity] *= 2);
	}

	(da[da_items])[da[da_count]-1] = val;
}

main() extrn printf, srand, rand, time; {
	auto nums[da_sizeof], i;

	srand(time(0));

	da_init(nums);

	i = 10; while (i --> 0) da_append(nums, rand() % 90 + 10);

	i = 0;
	while (i < nums[da_count]) {
		printf("nums[%d] = %d*n", i, (nums[da_items])[i]);
		++i;
	}

	da_deinit(nums);
}
