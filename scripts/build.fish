#!/bin/env fish

set BASE_DIR (realpath --relative-base (pwd) (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"

source $SCRIPTS_DIR/utils.fish

which bear &> /dev/null
and run_cmd -- bear -- make --directory "'$BASE_DIR'" $argv
or run_cmd -- make --directory "'$BASE_DIR'" $argv
