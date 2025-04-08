#!/bin/env fish

set BASE_DIR (realpath (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set BIN_DIR "$BASE_DIR/build/debug"


source "$SCRIPTS_DIR/utils.fish"

begin
	set --local CMD "$BIN_DIR/test_performance"
	test -f "$CMD"
	or error "'$CMD' not found. Did you run '$SCRIPTS_DIR/build.fish'?"
	and run_cmd --no-exit -- "'$CMD'"
	set --local EXIT_STATUS $status
	if test ! $EXIT_STATUS -eq 0
		echo "FAILURE: test failed"
	else
		echo "SUCCESS"
	end
	exit $EXIT_STATUS
end
