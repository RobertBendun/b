loud1() { extrn printf; printf("1,"); return(1); }
loud2() { extrn printf; printf("2,"); return(2); }
loud3() { extrn printf; printf("3,"); return(3); }
loud4() { extrn printf; printf("4,"); return(4); }

main() {
	extrn printf;
	loud1() * loud2() + loud3(); printf("*n");
	loud1() + loud2() * loud3(); printf("*n");
}
