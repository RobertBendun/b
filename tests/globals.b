a;
b 10;

/* array is by default initialized to 0 */
c[3];

/* if array count is larger then elements count, then missing elements are =0 */
d[4] 1, 2;

/* size of the array is max of elements and provided count */
e[3] 1, 2, 3, 4, 5;

/* one can store without arrays */
f 1, 2, 3, 4;

print() extrn printf; {
	printf("a: %d, b: %d, c: %d, %d, %d*n", a, b, c[0], c[1], c[2]);

	printf("d: %d, %d, %d, %d*n", d[0], d[1], d[2], d[3]);
	printf("e: %d, %d, %d, %d, %d*n", e[0], e[1], e[2], e[3], e[4]);
	printf("f: %d, %d, %d, %d*n", f, (&f)[1], (&f)[2], (&f)[3]);
}

main() {
	a = 20;
	++b;

	c[0] = 1;
	c[1] = 2;
	c[2] = 3;

	print();
}

