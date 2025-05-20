terminal_type="svg"
if( exists("terminal_type_arg") ) terminal_type=terminal_type_arg
set terminal terminal_type \
		size 800,600 \
		background rgb 'white'
if( exists("outputfile")) set output outputfile
set xlabel "iteration"
set ylabel "time in s"
if( exists("ylabel_arg") ) set ylabel ylabel_arg
# bg border, ticks, etc:
set style line 11 lc rgb '#808080' lt 1
set border 3 back ls 11
set tics nomirror
set style line 12 lc rgb '#808080' lt 0 lw 1
set grid back ls 12
# plot:
set style line 1 \
		linecolor rgb '#00acdc' \
		linetype 1 linewidth 1 \
		pointtype 7 pointsize .3
if( exists("title_arg")) set title title_arg
plot "/dev/stdin" title "" with points linestyle 1
