#!/usr/bin/awk -f

BEGIN {
	regex=ARGV[1] ".*START"
	delete ARGV[1]
}


$0 ~ regex {
	val=$(NF-1)
	sub(/:/,"",val)
	val -= int(val)
	print val
}
