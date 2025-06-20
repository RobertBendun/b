#!/usr/bin/env bash

set -ex -o pipefail

rm -fv b.gcda b.gcno
CFLAGS=--coverage make -B test
lcov --directory . --capture --output-file /tmp/coverage.info
genhtml --demangle-cpp -o /tmp/coverage /tmp/coverage.info
xdg-open /tmp/coverage/b/b.c.gcov.html
