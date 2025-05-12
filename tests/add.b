main() {
	extrn printf;
	auto a;
	auto b;
	printf("42 + 43 = %d*n", 42 + 43);

	a = 42;
	a = a + 43;
	printf("42 + 43 = %d*n", a);

	a = 42;
	b = 43;
	a = a + b;
	printf("42 + 43 = %d*n", a);
}
