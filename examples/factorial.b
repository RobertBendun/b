factorial(n) {
	if (n)
		return(factorial(n - 1) * n);
	return(1);
}

main() {
	extrn printf;
	printf("5! = %d*n", factorial(5));
}
