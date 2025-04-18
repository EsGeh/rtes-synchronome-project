#!/bin/env fish

set BASE_DIR (realpath (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"

source $SCRIPTS_DIR/utils.fish


run_cmd make $argv
