#!/bin/env fish

set BASE_DIR (realpath --relative-base (pwd) (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set DOC_DIR "$BASE_DIR/doc"
set OUTPUT_DIR "$BASE_DIR/doc/pdf"

source $SCRIPTS_DIR/utils.fish

run_cmd -- mkdir --parents "'$OUTPUT_DIR'"

run_cmd -- cd "'$DOC_DIR'"
run_cmd -- pandoc \
	--filter pandoc-plantuml \
	-o "'$OUTPUT_DIR/design.pdf'" \
	"'$DOC_DIR/imgs/pandoc_metadata.yaml'" \
	0_requirements.md \
	1_system-design.md \
	2_scheduling-and-timing-analysis.md 
run_cmd -- rm -r plantuml-images
