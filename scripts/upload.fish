#!/usr/bin/env fish

set BASE_DIR (realpath (status dirname)"/..")
set CONFIG "$BASE_DIR/local/config/remote.conf"
set SRC_DIR "$BASE_DIR"

echo "--------------------------"
echo "BASE_DIR: '$BASE_DIR'"

# echo "load config from '$CONFIG'"
if test -e "$CONFIG"
	source "$CONFIG"
else
	echo "'$CONFIG' does not exist." >&2
	exit 1
end

echo "loaded from '$CONFIG':"
echo "REMOTE: '$REMOTE'"
echo "REMOTE_DIR: '$REMOTE_DIR'"
echo "--------------------------"

function run_cmd
	echo "\$ $argv"
	$argv
end

run_cmd ssh $REMOTE \
	mkdir --parents "$REMOTE_DIR"

run_cmd rsync \
	--delete \
	--recursive \
	"$SRC_DIR/" \
	"$REMOTE:$REMOTE_DIR/"

ssh -t $REMOTE "cd $REMOTE_DIR && exec \$SHELL"
