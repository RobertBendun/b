main() {
	extrn printf;
	auto a;
	auto b;
	auto t;

	a = 41;
	b = 42;

	printf("before: %d %d*n", a, b);

	t = a;
	a = b;
	b = t;

	printf("after:  %d %d*n", a, b);
}
