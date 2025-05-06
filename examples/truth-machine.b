/* Inspired by https://esolangs.org/wiki/Truth-machine */
main() extrn putchar; putchar('y');

/* loops are not supported yet so real implementation is commented out */
/*
main()
	extrn putchar, getchar;
	auto c;
	if ((c = getchar()) == '0') putchar('0');
	else if (c == '1')          while (1) putchar('1');
	else                        return 1;
*/
