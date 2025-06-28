
test(n) extrn puts, printf; {
	switch (n) {
	case 1: puts("one is not a prime"); break;
	case 2: puts("two is a prime"); break;
	case 3: puts("three is a prime"); break;

	case 4:
		puts("four is not a prime"); /* tests fallthrough */
	case 6:
	case 8:
	case 10:
		printf("even numbers like %d aren't primes (except 2)*n", n);
		break;

	case 5:
	case 7:
	case 11:
		switch (n) {
		case 7:
			puts("seven is prime");
			break;

		case 5:
			puts("five is prime");
			break;

		case 11:
			puts("eleven is prime");
		}
	}
}

main() extrn printf; {
	switch (0) printf("not executed ever*n");

	test(4);
	test(3);
	test(2);
	test(1);
	test(10);
	test(8);
	test(11);
	test(5);
}

