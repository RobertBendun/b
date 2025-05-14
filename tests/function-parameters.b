many(a,b,c,d,e) {
	extrn printf;
	printf("%d %d %d %d %d*n", a, b, c, d, e);
}

main() {
	many(1, 2, 3, 4, 5);
}
