dump(n) extrn printf;
	printf("%d, %x, %o*n", n, n, n);


main() {
	dump(12_3456_7890);
	dump(0xff);
	dump(0XFF);

	dump(0644);
	dump(0o644);
}
