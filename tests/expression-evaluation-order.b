loud(n) { extrn printf; printf("%d,", n); return(n); }

main() {
	extrn printf;
	loud(1) * loud(2) + loud(3); printf("*n");
	loud(1) + loud(2) * loud(3); printf("*n");
}
