loud1() { extrn puts; puts("1"); return(1); }
loud2() { extrn puts; puts("2"); return(2); }
loud3() { extrn puts; puts("3"); return(3); }
loud4() { extrn puts; puts("4"); return(4); }

main() {
	extrn printf;
	printf("%d -> %d -> %d -> %d*n", loud1(), loud2(), loud3(), loud4());
}
