#!/bin/env fish

set BASE_DIR (realpath --relative-base (pwd) (status dirname)/..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set DOC_DIR "$BASE_DIR/doc"
set OUTPUT_DIR "$BASE_DIR/doc/pdf"

source $SCRIPTS_DIR/utils.fish

run_cmd -- mkdir --parents "'$OUTPUT_DIR'"

set OUTPUT_DIR (realpath --relative-to "$DOC_DIR" "$OUTPUT_DIR")

run_cmd -- $SCRIPTS_DIR/statistics/create_statistics.fish

run_cmd -- cd "'$DOC_DIR'"
run_cmd -- pandoc \
	--data-dir "utils/pandoc/data-dir" \
	--template eisvogel \
	--filter pandoc-plantuml \
	--toc \
	--number-sections \
	--listings \
	-o "'$OUTPUT_DIR/design.pdf'" \
	"'utils/pandoc/pandoc_metadata.yaml'" \
	0_requirements.md \
	1_system-design.md \
	2_scheduling-and-timing-analysis.md \
	3_conclusion.md
run_cmd -- rm -r plantuml-images
