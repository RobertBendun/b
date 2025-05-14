loud(n) { extrn printf; printf("%d*n", n); return(n); }

main() {
	extrn printf;
	printf("%d -> %d -> %d -> %d*n", loud(1), loud(2), loud(3), loud(4));
}
