main() {
	extrn printf;
	auto a, p;

	printf("-1 = %d*n", -1);

	a = 1; a = -a; printf("-1 = %d*n", a);

	p = &a; *p = -*p; printf(" 1 = %d*n", a);

	a = 0; printf(" 1 = %d*n", ++a);
	a = 2; printf(" 1 = %d*n", --a);

	p = &a; a = 0; ++*p; printf(" 1 = %d*n", a);
	p = &a; a = 2; --*p; printf(" 1 = %d*n", a);

	a = 0; a = !a; printf(" 1 = %d*n", a);
	a = 1; a = !a; printf(" 0 = %d*n", a);

	a = 0xff; a = ~a; printf(" ffffffffffffff00 = %llx*n", a);
	a = 0xffffffffffffff00; a = ~a; printf(" ff = %llx*n", a);
}
