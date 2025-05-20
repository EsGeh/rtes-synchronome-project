#!/usr/bin/awk -f

BEGIN {
	regex=ARGV[1] ".*RUNTIME"
	delete ARGV[1]
}

$0 ~ regex {
	print $(NF)
}
