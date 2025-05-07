main() {
	{
		extrn puts;
		/* here puts is visible */
		puts("hello");
		{
			/* puts is still visible */
			puts("hello");
		}
	}
	/* here puts IS NOT visible */
	puts("hello");
}
