#!/bin/env fish

set BASE_DIR (realpath --relative-base (pwd) (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set CONFIG debug
set BIN_DIR "$BASE_DIR/build/$CONFIG"
set OUTPUT_DIR "$BASE_DIR/local/output/synchronome"


source "$SCRIPTS_DIR/utils.fish"

run_cmd -- mkdir --parents "'$OUTPUT_DIR'"

begin
	set --local CMD "$BIN_DIR/synchronome"
	test -f "$CMD"
	or error "'$CMD' not found. Did you run '$SCRIPTS_DIR/build.fish'?"
	and run_cmd -- sudo "'$CMD'" (string escape -- $argv)
end
