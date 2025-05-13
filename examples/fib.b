main() {
	extrn printf;
	auto a;
	auto b;
	auto i;
	auto t;

	a = 1;
	b = 0;
	i = 1;

	while (i < 40) {
		printf("fib(%d) = %d*n", i, a);
		i = i + 1;
		t = a;
		a = a + b;
		b = t;
	}
}
