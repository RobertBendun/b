/* TODO: Support EOF */
main() extrn getchar; extrn putchar; {
	putchar(getchar());
	main();
}
