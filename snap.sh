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


if ./b <"$1" >"${asm_path}" 2>"${com_stderr}"; then
	echo "not implemented yet"
else
	if ! diff "${com_stderr}" "$1.com_stderr"; then
		exit 1
	fi
fi


rm -f "${com_stderr}" "${asm_path}" "${obj_path}" "${exe_path}"
