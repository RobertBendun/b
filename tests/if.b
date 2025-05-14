main() {
	extrn puts;
	if (1) puts("printed"); else puts("not printed");
	if (0) puts("not printed"); else puts("printed");

	if (0) puts("not printed");
	if (1) puts("printed");
}
