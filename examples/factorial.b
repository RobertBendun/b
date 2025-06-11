factorial(n) return(n > 1 ? n * factorial(n-1) : 1);

main() {
	extrn printf;
	printf("5! = %d*n", factorial(5));
}
