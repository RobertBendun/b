main() {
	auto a;
	extrn printf, sleep;

	a = 10;
	while (a >= 0) {
		printf("%d*n", a--);
		sleep(1);
	}
}
