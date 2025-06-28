main() extrn printf; {
	auto n, m;
	n = 5; while (n --> 0) printf("%d*n", n);
	n = 5; while (1) if (n --> 0) printf("%d*n", n); else break;

	n = 10; while (n --> 0) {
		if (n % 2 == 0) continue;
		printf("%d*n", n);
	}

	n = 10; while (n --> 0) {
		m = 10; while (m --> 0) {
			if (n*m % 2 == 0) continue;
			printf("%d ** %d = %d*n", n, m, n *m);
			if (n < 3) break;
		}

	}
}
