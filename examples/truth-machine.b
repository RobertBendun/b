/* Implementation of https://esolangs.org/wiki/Truth-machine */
main() {
	extrn getchar, putchar;
	if (getchar() == '1') {
		while (1) putchar('1');
	} else {
		putchar('0');
		putchar('*n');
	}
}
