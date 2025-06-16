exchange(dst, new) {
	auto old;
	old = *dst;
	*dst = new;
	return(old);
}

main() {
	extrn printf;
	auto a, b, i, t;
	a = 1;
	i = b = 0;

	while (++i < 40) {
		printf("fib(%d) = %d*n", i, a);
		b = exchange(&a, a + b);
	}
}
