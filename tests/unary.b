main() {
	extrn printf;
	auto a, p;

	printf("-1 = %d*n", -1);

	a = 1; a = -a; printf("-1 = %d*n", a);

	p = &a; *p = -*p; printf(" 1 = %d*n", a);
}
