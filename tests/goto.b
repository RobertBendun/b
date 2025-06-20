main() extrn printf; {
	auto n;

	n = 0;
	again: if (n < 10) {
		printf("%d*n", n++);
		goto again;
	}
}
