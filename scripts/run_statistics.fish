#!/bin/env fish

set BASE_DIR (realpath --relative-base (pwd) (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set CONFIG release
set BIN_DIR "$BASE_DIR/build/$CONFIG"
set OUTPUT_DIR "$BASE_DIR/local/output/statistics/imgs"


source "$SCRIPTS_DIR/utils.fish"

true
and run_cmd -- make --directory "'$BASE_DIR'" CONFIG=$CONFIG
and run_cmd -- mkdir --parents "'$OUTPUT_DIR'"

begin
	set --local CMD "$BIN_DIR/statistics"
	test -f "$CMD"
	or error "'$CMD' not found. Did you run '$SCRIPTS_DIR/build.fish'?"
	and run_cmd -- "'$CMD'" (string escape -- $argv)
end
