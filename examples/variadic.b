/* NOTE: This function can handle as many variadic parameters
         as mentioned in the signature */
max(x0,x1,x2,x3,x4,x5) extrn printf; {
	auto i, p, m;
	p = &x0;
	m = x0;

	i = 0;
	while (p[i] != 0) {
		if (p[i] > m) m = p[i];
		++i;
	}

	return(m);
}


main() extrn printf; {
	printf("max(1, 2) = %lld*n", max(1, 2, 0));
	printf("max(1, 3, 2) = %lld*n", max(1, 3, 2, 0));
}
