/* example work in progress */

/* TODO: errno isn't available due to beeing thread local */

AF_INET 2;
SOCK_STREAM 1;
SOL_SOCKET 1;
SO_REUSEADDR 2;

/*
Generated from C code:
	struct sockaddr_in sin = {};
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(8080);
	uint64_t *n = (void*)&sin;
	printf("sockaddr_in %llu, %llu\n", n[0], n[1]);
	printf("sockaddr_in_size %zu\n", sizeof(sin));
*/
sockaddr_in 2417950722, 0;
sockaddr_in_size 16;

/* match 16 byte width of sockaddr_in */
client_sockaddr 0, 0;

/* TODO: Mechanism for constants */
buf[128];
bufsize 1024; /* 128 * wordsize = 128 * 8 = 1024 */

page "<!DOCTYPE html>
<html>
	<head>
		<meta charset=*"utf-8*">
		<title>Hello from B</title>
	</head>
	<body>
		<h1>Hello from B</h1>
	</body>
</html>";

main() {
	/* posix */ extrn socket, bind, close, listen, accept, read, setsockopt;
	/* libc  */ extrn perror, printf, fprintf, fdopen, fflush, strlen, strstr;
	auto server, client;

	server = socket(AF_INET, SOCK_STREAM, 0);
	if (!server) {
		perror("failed to open a socket");
		return(1);
	}

	auto opt;
	opt = 1;
	if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, &0[1]) != 0) {
		perror("failed to set SO_REUSEADDR");
		return(1);
	}

	if (bind(server, &sockaddr_in, sockaddr_in_size) != 0) {
		perror("failed to bind socket");
		return(1);
	}

	if (listen(server, 1) != 0) {
		perror("failed to listen on a socket");
		return(1);
	}

	while ((client = accept(server, 0, 0)) >= 0) {
		auto r;
		while (1) {
			r = read(client, buf, bufsize - 1);
			if (r < 0) {
				perror("failed to read");
				break;
			} else if (r == 0) {
				break;
			}
			if (strstr(buf, "*r*n*r*n")) break;
		}

		auto f;
		f = fdopen(client, "w");
		fprintf(f, "HTTP/1.1 200 OK*r*n");
		fprintf(f, "Content-Type: %s*r*n", "text/html; charset=utf-8");
		fprintf(f, "Content-Length: %d*r*n", strlen(page));
		fprintf(f, "*r*n");
		fprintf(f, "%s", page);
		fflush(f);
		close(client);
	}

	close(server);
}
