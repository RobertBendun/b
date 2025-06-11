main(argc, argv) {
	extrn printf;

	auto i;
	i = 1;

	while (i < argc) {
		printf("%s", *(argv + i * 8));
		i += 1;
		if (i != argc) {
			printf(" ");
		}
	}
	printf("*n");
}
