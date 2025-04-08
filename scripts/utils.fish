########################
# Util Functions
########################

function run_cmd
	argparse 'n/no-exit' -- $argv
	or error "internal error in 'run_cmd'"
	echo "\$ $argv"
	if test "$DRY_RUN" != "1"
		eval $argv
		set --local EXIT_STATUS $status
		if test $EXIT_STATUS -eq 0
			return 0
		else
			if set --query _flag_no_exit
				return $EXIT_STATUS
			else
				exit $EXIT_STATUS
			end
		end
	end
end

function error
	echo "ERROR: "$argv >&2
	exit 1
end

function check_exe
	set --local exe $argv[1]
	which $exe &> /dev/null
	and begin
		echo "[x] - found command $exe"
		return
	end
	or error "'$exe' not found! Is it installed?"
end

function read_confirm
	set --local prompt_str 
	if test (count $argv) -gt 0
		set prompt_str $argv
	end
  while true 'Do you want to continue?'
    read --local --prompt-str "$prompt_str [y/N] " confirm
    switch $confirm
      case Y y
        return 0
      case '' N n
        return 1
    end
  end
end
