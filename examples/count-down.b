main() {
	auto a;
	extrn printf, sleep;

	a = 10;
	while (a) {
		printf("%d*n", a);
		a -= 1;
		sleep(1);
	}
}
