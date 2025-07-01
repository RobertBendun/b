/* example work in progress */
/* usage: examples/http [port] */

/* TODO: errno isn't available due to beeing thread local */

AF_INET 2;
SOCK_STREAM 1;
SOL_SOCKET 1;
SO_REUSEADDR 2;
INADDR_ANY 0;

sockaddr_in 0, 0;
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

sin_family_offset 0; /* u16 */
sin_addr_offset 4;   /* u32 */
sin_port_offset 2;   /* u16 */

u16_mask 0xffff;
u32_mask 0xffff_ffff;

/* TODO: Some support for writing to C structs */
sockaddr_in_new(this, sin_family, sin_addr, sin_port) {
	this[0] = this[1] = 0;
	this[0] |= (sin_family & u16_mask) << sin_family_offset*8;
	this[0] |= (sin_addr   & u32_mask) << sin_addr_offset*8;
	this[0] |= (sin_port   & u16_mask) << sin_port_offset*8;
}

http_new(port) {
	extrn socket, setsockopt, bind, listen, htonl, htons;
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

	sockaddr_in_new(&sockaddr_in, AF_INET, htonl(INADDR_ANY), htons(port));

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

log_time() extrn time, localtime, strftime, printf; {
	auto t, tm, timebuf[3]; /* strlen("0000-00-00 00:00:00") / 8 = 22 / 8 = 3 */
	t = time(0);
	tm = localtime(&t);
	strftime(timebuf, 3 * &0[1], "%Y-%m-%d %H:%M:%S", tm);
	printf("[%s] ", timebuf);
}

main(argc, argv) {
	extrn close;
	extrn strlen, printf, sscanf, stderr, fprintf;
	auto server, client, method, url, body, resp, port;

	if (argc == 2) {
		if (sscanf(argv[1], "%lld", &port) != 1) {
			fprintf(stderr, "error reading port number. expected integer, got %s*n", argv[1]);
			return(1);
		}
	} else {
		port = 8080;
	}

	server = http_new(port);
	log_time(); printf("Listening on http://localhost:%d*n", port);

	while (http_accept_request(server, &client, &method, &url, &body)) {
		log_time(); printf("%s %s*n", method, url);

		resp = http_response_begin(client, 200);
		http_response_content_type(resp, "text/html; charset=utf-8");
		http_response_body(resp, page, strlen(page));
		http_response_end(resp);
	}

	close(server);
}
