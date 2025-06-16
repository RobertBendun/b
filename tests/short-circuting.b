main() extrn puts, printf; {
	auto a, b;

	a = 0; b = 0; printf("%d && %d = %d*n", a, b, a && b);
	a = 0; b = 1; printf("%d && %d = %d*n", a, b, a && b);
	a = 1; b = 0; printf("%d && %d = %d*n", a, b, a && b);
	a = 1; b = 1; printf("%d && %d = %d*n", a, b, a && b);

	a = 0; b = 0; printf("%d || %d = %d*n", a, b, a || b);
	a = 0; b = 1; printf("%d || %d = %d*n", a, b, a || b);
	a = 1; b = 0; printf("%d || %d = %d*n", a, b, a || b);
	a = 1; b = 1; printf("%d || %d = %d*n", a, b, a || b);

	0 && puts("not printed");
	1 && puts("printed");

	0 || puts("printed");
	1 || puts("not printed");

	1 && 0 && puts("not printed");
	1 && 1 && puts("printed");

	0 || 0 || puts("printed");
	0 || 1 || puts("not printed");

	if (0 || 0) puts("not printed");
	if (0 || 1) puts("printed");
	if (1 || 0) puts("printed");
	if (1 || 1) puts("printed");

	if (1 > 10 || 1 < 10) puts("printed");
}
