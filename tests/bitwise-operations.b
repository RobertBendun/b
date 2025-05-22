main() {
	extrn printf;
	auto a, b;

	a = 0; b = 0; printf("%d | %d = %d*n", a, b, a | b);
	a = 0; b = 1; printf("%d | %d = %d*n", a, b, a | b);
	a = 1; b = 0; printf("%d | %d = %d*n", a, b, a | b);
	a = 1; b = 1; printf("%d | %d = %d*n", a, b, a | b);
	a = 1; b = 2; printf("%d | %d = %d*n", a, b, a | b);

	a = 0; b = 0; printf("%d ^ %d = %d*n", a, b, a ^ b);
	a = 0; b = 1; printf("%d ^ %d = %d*n", a, b, a ^ b);
	a = 1; b = 0; printf("%d ^ %d = %d*n", a, b, a ^ b);
	a = 1; b = 1; printf("%d ^ %d = %d*n", a, b, a ^ b);
	a = 1; b = 2; printf("%d ^ %d = %d*n", a, b, a ^ b);

	a = 0; b = 0; printf("%d & %d = %d*n", a, b, a & b);
	a = 0; b = 1; printf("%d & %d = %d*n", a, b, a & b);
	a = 1; b = 0; printf("%d & %d = %d*n", a, b, a & b);
	a = 1; b = 1; printf("%d & %d = %d*n", a, b, a & b);
	a = 1; b = 2; printf("%d & %d = %d*n", a, b, a & b);
}
