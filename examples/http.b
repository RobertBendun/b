/* example work in progress */
/* usage: examples/http [port] */

/* TODO: errno isn't available due to beeing thread local */

AF_INET 2;
SOCK_STREAM 1;
SOL_SOCKET 1;
SO_REUSEADDR 2;
INADDR_ANY 0;
O_RDONLY 0;

S_IFMT     0170000; /* mask to extract file mode bits */
S_IFSOCK   0140000; /* socket */
S_IFLNK    0120000; /* symbolic link */
S_IFREG    0100000; /* regular file */
S_IFBLK    0060000; /* block device */
S_IFDIR    0040000; /* directory */
S_IFCHR    0020000; /* character device */
S_IFIFO    0010000; /* FIFO */


sockaddr_in_size 16;

sin_family_offset 0; /* u16 */
sin_addr_offset 4;   /* u32 */
sin_port_offset 2;   /* u16 */

stat_size 144;
stat_st_mode_offset 24; /* u32 */
stat_st_size_offset 48; /* u64 */

/* for pointer to struct stat return if it's a particular type */
S_ISDIR(stat) extrn u32; return((u32(stat+stat_st_mode_offset) & S_IFMT) == S_IFDIR);

stat_st_size(stat) return(*(stat+stat_st_size_offset));

/* TODO: Mechanism for constants */

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

page404 "<!DOCTYPE html>
<html>
	<head>
		<meta charset=*"utf-8*">
		<title>Not Found</title>
	</head>
	<body>
		<h1>Page Not Found</h1>
	</body>
</html>";

page405 "<!DOCTYPE html>
<html>
	<head>
		<meta charset=*"utf-8*">
		<title>Method Not Allowed</title>
	</head>
	<body>
		<h1>Method Not Allowed</h1>
	</body>
</html>";



exchange(p, new) {
	auto old;
	old = *p;
	*p = new;
	return(old);
}

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
	auto server, opt, sockaddr_in[2];

	server = socket(AF_INET, SOCK_STREAM, 0);
	if (!server) {
		perror("failed to open a socket");
		return(1);
	}

	opt = 1;
	if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, &0[1]) != 0) {
		perror("failed to set SO_REUSEADDR");
		return(1);
	}

	sockaddr_in_new(sockaddr_in, AF_INET, htonl(INADDR_ANY), htons(port));

	if (bind(server, sockaddr_in, sockaddr_in_size) != 0) {
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

buf[128];

http_accept_request(server, clientp, method, url, body) {
	/* posix */ extrn accept, read;
	/* libc  */ extrn strstr, perror, strncmp, fprintf, stderr, memchr, memset, printf;
	/* libb  */ extrn i8set;
	auto client, bufsize;
	bufsize = 128 * 8;

	if ((client = accept(server, 0, 0)) < 0) return(0);

	auto r, p, sz, endp;

	memset(buf, 0, bufsize);
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

	if (!(endp = memchr(p, ' ', sz))) return(0);
	i8set(endp, 0);

	*url = exchange(&p, endp);
	*body = "";
	*clientp = client;
	return(1);
}

http_status_code_to_message(code) {
	extrn fprintf, exit, stderr;

	switch (code) {
	case 200: return("OK");
	case 404: return("Not Found");
	case 405: return("Method Not Allowed");
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

dirent_size 280; /* 35 words */
dirent_d_type_offset 18; /* 1 bytes */
dirent_d_name_offset 19;

append(x, y)
	extrn strlen, calloc, realloc, strdup, strcat;
	return (x ? strcat(realloc(x, strlen(x) + strlen(y) + 1), y) : strdup(y));


/* TODO: sorted output */
directory_response(fd, client, url) {
	extrn close;
	extrn fdopendir, closedir, readdir, printf, strlen;
	auto dir, dirent, out, resp;

	dir = fdopendir(fd);

	out = 0;
	out = append(out, "<!DOCTYPE html><html><head><meta charset=*"utf-8*"><title>");
	out = append(out, url);
	out = append(out, "</title></head><body><ul>");


	while (dirent = readdir(dir)) {
		out = append(out, "<li><a href=*"");
		out = append(out, url);
		out = append(out, "/");
		out = append(out, dirent + dirent_d_name_offset);
		out = append(out, "*">");
		out = append(out, dirent + dirent_d_name_offset);
		out = append(out, "</a></li>");
	}
	out = append(out, "</ul></body></html>");

	resp = http_response_begin(client, 404);
	http_response_content_type(resp, "text/html; charset=utf-8");
	http_response_body(resp, out, strlen(out));
	http_response_end(resp);

	closedir(dir);
}

statbuf[18];
copybuf[128];
try_file_response(client, url) {
	/* posix */ extrn open, fstat, write;
	/* libc  */ extrn printf, strlen, perror, fdopen, fprintf, fread, fwrite, fclose, fflush, malloc, strcat, strcmp;
	/* libb  */ extrn i8set;
	auto resp, fd, size, read, p; /* 144 / 8 = 18 */


	p = malloc(strlen(url) + 2);
	i8set(p+0, '.');
	i8set(p+1, 0);
	url = strcat(p, url);


	if ((fd = open(p, O_RDONLY)) < 0) {
		return(0);
	}

	if (fstat(fd, statbuf) < 0) {
		perror("fstat");
		return(0);
	}

	printf(" -> 200 %s*n", http_status_code_to_message(200));

	if (S_ISDIR(statbuf)) {
		directory_response(fd, client, url);
		return(1);
	}

	resp = http_response_begin(client, 200);
	/* TODO: Detect file type */
	http_response_content_type(resp, "text/plain");

	fd = fdopen(fd, "r");
	if (!fd) {
		perror("failed to reopen file");
		return(1);
	}
	size = stat_st_size(statbuf);
	fprintf(resp, "Content-Length: %llu*r*n*r*n", size);

	while (size > 0) {
		if ((read = fread(copybuf, 1, 128 * &0[1], fd)) <= 0) {
			perror("fread");
			fclose(fd);
			http_response_end(resp);
			return(1);
		}
		/* TODO: verify that we indeed written */
		if (read > 0 && !fwrite(copybuf, 1, read, resp)) {
			perror("fwrite");
			fclose(fd);
			http_response_end(resp);
			return(1);
		}
		size -= read;
	}

	fclose(fd);
	http_response_end(resp);
	return(1);
}

main(argc, argv) {
	extrn close;
	extrn strlen, strcmp, printf, sscanf, stderr, fprintf;
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

	client = method = url = body = 0;
	while (http_accept_request(server, &client, &method, &url, &body)) {
		log_time(); printf("%s %s", method, url);

		if (strcmp(method, "GET") == 0) {
			if (try_file_response(client, url)) {
				continue;
			}

			printf(" -> 404 %s*n", http_status_code_to_message(404));
			resp = http_response_begin(client, 404);
			http_response_content_type(resp, "text/html; charset=utf-8");
			http_response_body(resp, page404, strlen(page404));
			http_response_end(resp);
			continue;
		}

		printf(" -> 405 %s*n", http_status_code_to_message(405));
		resp = http_response_begin(client, 405);
		http_response_content_type(resp, "text/html; charset=utf-8");
		http_response_body(resp, page405, strlen(page405));
		http_response_end(resp);
	}

	close(server);
}
