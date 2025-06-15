main() {
	extrn printf;
	auto a, b, i, t;
	a = 1;
	i = b = 0;
	while (++i < 40) {
		printf("fib(%d) = %d*n", i, a);
		t = a;
		a += b;
		b = t;
	}
}
