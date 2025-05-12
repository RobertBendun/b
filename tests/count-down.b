main() {
	auto a;
	extrn printf;

	a = 10;
	while (a) {
		printf("%d*n", a);
		a = a - 1;
	}
}
