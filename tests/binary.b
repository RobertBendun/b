main() extrn printf; {
	printf("2 << 2 = %d*n", 2 << 2);
	printf("64 >> 2 = %d*n", 64 >> 2);

	auto a;
	a = 2; a <<= 2; printf("2 << 2 = %d*n", a);
	a = 64; a >>= 2; printf("64 >> 2 = %d*n", a);
}
