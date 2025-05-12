test() {
	extrn puts;
	puts("test: this one is printed");
	return;
	puts("test: and this one is not");
}

main() {
	extrn puts;
	puts("main: this one is printed");
	test();
	return(0);
	puts("main: and this one is not");
}
