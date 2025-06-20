
max(a, b) return(a > b ? a : b);


sign(n) return(
	n > 0 ?  1 :
	n < 0 ? -1 :
	         0);


main() {
	extrn printf;

	printf("max 10, 20 = %d*n", max(10, 20));
	printf("max 20, 10 = %d*n", max(20, 10));

	/* side effects: */

	0 ? printf("not printed*n") : printf("printed*n");
	1 ? printf("printed*n") : printf("not printed*n");

	printf("sign 10 = %d*n", sign(10));
	printf("sign -10 = %d*n", sign(-10));
	printf("sign 0 = %d*n", sign(0));
}
