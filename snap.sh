#!/usr/bin/env bash

set -o pipefail

if [ -z "$1" ]; then
	1>&2 echo "[ERROR] Please provide file to test as an argument"
	exit 1
fi

com_stderr="$(mktemp)"
asm_path="$(mktemp tmp.XXXXX.asm -p /tmp/)"
obj_path="$(mktemp tmp.XXXXX.o -p /tmp/)"
exe_path="$(mktemp)"
run_stdout="$(mktemp)"
run_stderr="$(mktemp)"


if ./b <"$1" >"${asm_path}" 2>"${com_stderr}"; then
	if ! fasm "${asm_path}" "${obj_path}" >/dev/null; then
		exit 1
	fi
	if ! gcc -o "${exe_path}" "${obj_path}"; then
		exit 1
	fi
	"$(realpath "${exe_path}")" >"${run_stdout}" 2>"${run_stderr}"
	exit_code="$?"

	if ! diff -N "${run_stdout}" "$1.run_stdout"; then exit 1; fi
	if ! diff -N "${run_stderr}" "$1.run_stderr"; then exit 1; fi
	if [ -f "$1.exit_code" ]; then
		if ! echo "${exit_code}" | diff - "$1.exit_code"; then exit 1; fi
	elif [ "${exit_code}" -ne 0 ]; then
		echo "Expected error code = 0, got ${exit_code}"
		exit 1
	fi
else
	if ! diff "${com_stderr}" "$1.com_stderr"; then
		exit 1
	fi
fi


rm -f "${com_stderr}" "${asm_path}" "${obj_path}" "${exe_path}"
