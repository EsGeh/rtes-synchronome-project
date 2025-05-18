#!/bin/env fish

set BASE_DIR (realpath --relative-base (pwd) (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set CONFIG test
set BUILD_DIR "$BASE_DIR/build/$CONFIG"
set COVERAGE_DIR "$BUILD_DIR/coverage"


source "$SCRIPTS_DIR/utils.fish"

true
and run_cmd -- make --directory "'$BASE_DIR'" clean
and run_cmd -- make --directory "'$BASE_DIR'" CONFIG=$CONFIG
and run_cmd -- mkdir --parents "'$COVERAGE_DIR'"

begin
	set --local CMD "$BUILD_DIR/run_tests"
	and run_cmd --no-exit -- "'$CMD'" (string escape -- $argv)
	set --local EXIT_STATUS $status
	# NOTE: errors in coverage report generation
	#   are considered non fatal
	# (NOTE: why does an absolute --directory option not work?)
	run_cmd --no-exit -- lcov (string escape -- --capture \
		--base-directory "$BASE_DIR" \
		--directory "'$BUILD_DIR/objs'" \
		--output-file $COVERAGE_DIR/coverage) '>/dev/null'
	run_cmd --no-exit -- genhtml (string escape -- --output-directory $COVERAGE_DIR/html $COVERAGE_DIR/coverage) '>/dev/null'
	if test ! $EXIT_STATUS -eq 0
		echo "FAILURE: test failed"
	else
		echo "SUCCESS"
	end
	exit $EXIT_STATUS
end
