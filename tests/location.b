foo() extrn printf; {
	printf("File: %s, line: %d, func: %s*n", __FILE__, __LINE__, __FUNCTION__);
}

main() extrn printf; {
	printf("File: %s, line: %d, func: %s*n", __FILE__, __LINE__, __FUNCTION__);
	foo();
}
