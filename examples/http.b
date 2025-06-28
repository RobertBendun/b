/* example work in progress */

/* TODO: errno isn't available due to beeing thread local */

AF_INET 2;
SOCK_STREAM 1;
SOL_SOCKET 1;
SO_REUSEADDR 2;

/*
TODO: Port specification (for now using bit operations I guess)
TODO: Some support for writing to C structs

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

exchange(p, new) {
	auto old;
	old = *p;
	*p = new;
	return(old);
}

http_new() {
	extrn socket, setsockopt, bind, listen;
	extrn printf, perror;
	auto server, opt;

	server = socket(AF_INET, SOCK_STREAM, 0);
	if (!server) {
		perror("failed to open a socket");
		return(1);
	}

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

	return(server);
}

min(a, b) return(a < b ? a : b);

remove_prefix_if_exists(s, len, prefix) extrn strncmp, strlen; {
	if (*len >= strlen(prefix) && strncmp(*s, prefix, strlen(prefix)) == 0) {
		*s += strlen(prefix);
		*len -= strlen(prefix);
		return(1);
	}
	return(0);
}

http_accept_request(server, clientp, method, url, body) {
	/* posix */ extrn accept, read;
	/* libc  */ extrn strstr, perror, strncmp, fprintf, stderr, memchr;
	auto client;

	if ((client = accept(server, 0, 0)) < 0) return(0);

	auto r, p, sz, endp;

	r = read(client, buf, bufsize - 1);
	if (r < 0) {
		perror("failed to read");
		return(0);
	} else if (r == 0) {
		return(0);
	}

	p = buf;
	sz = r;

	if (0) {}
	else if (remove_prefix_if_exists(&p, &sz, "DELETE")) { *method = "DELETE"; }
	else if (remove_prefix_if_exists(&p, &sz, "GET"))    { *method = "GET";    }
	else if (remove_prefix_if_exists(&p, &sz, "HEAD"))   { *method = "HEAD";   }
	else if (remove_prefix_if_exists(&p, &sz, "POST"))   { *method = "POST";   }
	else if (remove_prefix_if_exists(&p, &sz, "PUT"))    { *method = "PUT";    }
	else {
		fprintf(stderr, "Invalid request: missing method: %.4s*n", p);
		return(0);
	}

	++p; --sz; /* skip space */

	/* TODO: Verify that we indeed found ' ' */
	endp = memchr(p, ' ', sz);
	*endp = 0;
	*url = exchange(&p, endp);

	*body = "";
	*clientp = client;
	return(1);
}

http_status_code_to_message(code) {
	extrn fprintf, exit, stderr;

	switch (code) {
	case 200: return("OK");
	}

	fprintf(stderr, "ERROR: unknown error message code: %d*n", code);
	exit(1);
}

http_response_begin(client, code) {
	/* libc  */ extrn fdopen, fprintf;
	auto f;
	f = fdopen(client, "w");
	fprintf(f, "HTTP/1.1 %d %s*r*n", code, http_status_code_to_message(code));
	return(f);
}

http_response_content_type(f, type) extrn fprintf; fprintf(f, "Content-Type: %s*r*n", type);
http_response_body(f, body, len) extrn fprintf; fprintf(f, "Content-Length: %llu*r*n*r*n%.**s", len, len, body);
http_response_end(f) extrn fclose; fclose(f);

/* TODO: local arrays */
timebuf[3]; /* strlen("0000-00-00 00:00:00") / 8 = 22 / 8 = 3 */
timebuf_size 24;

log_time() extrn time, localtime, strftime, printf; {
	auto t, tm;
	t = time(0);
	tm = localtime(&t);
	strftime(timebuf, timebuf_size, "%Y-%m-%d %H:%M:%S", tm);
	printf("[%s] ", timebuf);
}

main() {
	extrn close;
	extrn strlen, printf;
	auto server, client, method, url, body, resp;


	server = http_new();
	log_time(); printf("Listening on http://localhost:8080*n");

	while (http_accept_request(server, &client, &method, &url, &body)) {
		log_time(); printf("%s %s*n", method, url);

		resp = http_response_begin(client, 200);
		http_response_content_type(resp, "text/html; charset=utf-8");
		http_response_body(resp, page, strlen(page));
		http_response_end(resp);
	}

	close(server);
}
