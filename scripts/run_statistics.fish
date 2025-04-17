#!/bin/env fish

set BASE_DIR (realpath (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set BIN_DIR "$BASE_DIR/build/debug"
set OUTPUT_DIR "$BASE_DIR/local/img"


source "$SCRIPTS_DIR/utils.fish"

run_cmd -- mkdir --parents "'$OUTPUT_DIR'"

begin
	set --local CMD "$BIN_DIR/statistics"
	test -f "$CMD"
	or error "'$CMD' not found. Did you run '$SCRIPTS_DIR/build.fish'?"
	and run_cmd -- "'$CMD'" (string escape -- $argv)
end
