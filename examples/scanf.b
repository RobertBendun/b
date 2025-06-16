main() {
	extrn scanf, printf;
	auto lhs, op, rhs;

	printf("Pass number op number: ");
	scanf("%d %c %d", &lhs, &op, &rhs);

	if (op == '+') { printf("%d*n", lhs + rhs); }
	else if (op == '-') { printf("%d*n", lhs - rhs); }
	else if (op == '**') { printf("%d*n", lhs * rhs); }
	else if (op == '/') { printf("%d / %d = %d*n", lhs, rhs, lhs / rhs); }
	else printf("Unknown operator %c*n", op);
}
