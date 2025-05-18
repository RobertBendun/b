main() {
	extrn printf, malloc, free;
	auto x, y;
	*&x = 10;

	y = malloc(2 * 8);
	*y = 20;
	*(y + 8) = 30;
	printf("%d %d %d*n", x, *y, *(y + 8));
	free(y);
}
