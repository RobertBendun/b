/* Implementation of https://esolangs.org/wiki/Truth-machine */
main() {
	extrn getchar;
	extrn putchar;
	auto c;
	c = getchar();
	if (c - '0') {
		while (1) putchar('1');
	} else {
		putchar('0');
		putchar('*n');
	}
}
