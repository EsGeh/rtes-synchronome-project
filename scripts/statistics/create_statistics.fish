#!/bin/env fish

set BASE_DIR (realpath --relative-base (pwd) (status dirname)/../..)
set SCRIPTS_DIR "$BASE_DIR/scripts"
set DOC_DIR "$BASE_DIR/doc"
set OUTPUT_DIR "$DOC_DIR/imgs/diagrams/statistics"

source $SCRIPTS_DIR/utils.fish

run_cmd -- mkdir --parents "'$OUTPUT_DIR'"

for service in frame_acq select convert write_to_storage
	for format in svg png
		set terminal_type_arg $format
		if test $format = 'png'
			set terminal_type_arg 'pngcairo'
		end
		set title (string replace --all "_" " " "$service start time")
		set output_file "$service"'_start'
		set ylabel 'time in fractions of s'
		run_cmd -- cat "$DOC_DIR/example_output/synchronome.log" \
			\| awk -f "$SCRIPTS_DIR/statistics/get_start.awk" $service \
			\| gnuplot \
				-e "'outputfile=\'$OUTPUT_DIR/$output_file.$format\''" \
				-e "'terminal_type_arg=\'$terminal_type_arg\''" \
				-e "'title_arg=\'$title\''" \
				-e "'ylabel_arg=\'$ylabel\''" \
				-c $SCRIPTS_DIR/statistics/plot.plt
		set title (string replace --all "_" " " "$service runtime")
		set output_file "$service"'_runtime'
		run_cmd -- cat "$DOC_DIR/example_output/synchronome.log" \
			\| awk -f "$SCRIPTS_DIR/statistics/get_runtime.awk" $service \
			\| gnuplot \
				-e "'outputfile=\'$OUTPUT_DIR/$output_file.$format\''" \
				-e "'terminal_type_arg=\'$terminal_type_arg\''" \
				-e "'title_arg=\'$title\''" \
				-c $SCRIPTS_DIR/statistics/plot.plt
	end
end
