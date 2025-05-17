main() {
	extrn printf;
	auto a, b, i, t;
	a = i = 1;
	b = 0;
	while (i < 40) {
		printf("fib(%d) = %d*n", i, a);
		i = i + 1;
		t = a;
		a = a + b;
		b = t;
	}
}
