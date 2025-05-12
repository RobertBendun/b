main() {
	extrn printf;
	auto a;
	auto b;
	auto t;

	a = 1; b = 1;

	t = a; a = a + b; b = t; printf("fib(1) = %d*n", b);
	t = a; a = a + b; b = t; printf("fib(2) = %d*n", b);
	t = a; a = a + b; b = t; printf("fib(3) = %d*n", b);
	t = a; a = a + b; b = t; printf("fib(4) = %d*n", b);
	t = a; a = a + b; b = t; printf("fib(5) = %d*n", b);
	t = a; a = a + b; b = t; printf("fib(6) = %d*n", b);
	t = a; a = a + b; b = t; printf("fib(7) = %d*n", b);
	t = a; a = a + b; b = t; printf("fib(8) = %d*n", b);
	t = a; a = a + b; b = t; printf("fib(9) = %d*n", b);
}
