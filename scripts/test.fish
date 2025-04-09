#!/bin/env fish

set BASE_DIR (realpath (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set BIN_DIR "$BASE_DIR/build/debug"
set COVERAGE_DIR "$BIN_DIR/coverage"


source "$SCRIPTS_DIR/utils.fish"

run_cmd -- make --directory "'$BASE_DIR'"
run_cmd -- mkdir --parents "'$COVERAGE_DIR'"

begin
	set --local CMD "$BIN_DIR/run_tests"
	test -f "$CMD"
	or error "'$CMD' not found. Did you run '$SCRIPTS_DIR/build.fish'?"
	and run_cmd --no-exit -- "'$CMD'" (string escape -- $argv)
	set --local EXIT_STATUS $status
	# NOTE: errors in coverage report generation
	#   are considered non fatal
	# (NOTE: why does an absolute --directory option not work?)
	run_cmd --no-exit -- lcov (string escape -- --capture \
		--base-directory "$BASE_DIR" \
		--directory build/debug/objs \
		--output-file $COVERAGE_DIR/coverage) '>/dev/null'
	run_cmd --no-exit -- genhtml (string escape -- --output-directory $COVERAGE_DIR/html $COVERAGE_DIR/coverage) '>/dev/null'
	if test ! $EXIT_STATUS -eq 0
		echo "FAILURE: test failed"
	else
		echo "SUCCESS"
	end
	exit $EXIT_STATUS
end
