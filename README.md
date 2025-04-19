# Synchronome Project (debugging vim/ale/clang-check)

***debugging vim / ale / clang-check issues:***
The sole purpose of this branch/commit is to give an example for issues with ale as an integrated linter for vim.

***Issue:***
When opening/saving a `*.c` file, clang-check will generate a corresponding `*.plist` file in the project dir with the current config.

The issue is propably related to: <https://github.com/dense-analysis/ale/issues/4703>

## Vim/Ale Config

    let g:ale_completion_enabled = 1
    let g:ale_completion_autoimport = 1

    let g:ale_c_build_dir_names = ['build', 'release', 'debug']
    let g:ale_c_cc_options = c_opts

## Output of :ALEInfo

     Current Filetype: c
    Available Linters: ['cc', 'ccls', 'clangcheck', 'clangd', 'clangtidy', 'cppcheck', 'cpplint', 'cquery', 'cspell', 'flawfinder']
       Linter Aliases:
    'cc' -> ['gcc', 'clang']
      Enabled Linters: ['cc', 'ccls', 'clangcheck', 'clangd', 'clangtidy', 'cppcheck', 'cpplint', 'cquery', 'cspell', 'flawfinder']
      Ignored Linters: []
     Suggested Fixers:
      'astyle' - Fix C/C++ with astyle.
      'clang-format' - Fix C, C++, C#, CUDA, Java, JavaScript, JSON, ObjectiveC and Protobuf files with clang-format.
      'clangtidy' - Fix C/C++ and ObjectiveC files with clang-tidy.
      'remove_trailing_lines' - Remove all blank lines at the end of a file.
      'trim_whitespace' - Remove all trailing whitespace characters at the end of every line.
      'uncrustify' - Fix C, C++, C#, ObjectiveC, ObjectiveC++, D, Java, Pawn, and VALA files with uncrustify.
     
     Linter Variables:
    " Press Space to read :help for a setting
    let g:ale_c_always_make = 1
    let g:ale_c_build_dir = ''
    let g:ale_c_build_dir_names = ['build', 'release', 'debug']
    let g:ale_c_cc_executable = '<auto>'
    let g:ale_c_cc_header_exts = ['h']
    let g:ale_c_cc_options = '-std=gnu23 -Wall -Wextra -pedantic'
    let g:ale_c_cc_use_header_lang_flag = -1
    let g:ale_c_ccls_executable = 'ccls'
    let g:ale_c_ccls_init_options = {}
    let g:ale_c_clangcheck_executable = 'clang-check'
    let g:ale_c_clangcheck_options = ''
    let g:ale_c_clangd_executable = 'clangd'
    let g:ale_c_clangd_options = ''
    let g:ale_c_clangtidy_checks = []
    let g:ale_c_clangtidy_executable = 'clang-tidy'
    let g:ale_c_clangtidy_extra_options = ''
    let g:ale_c_clangtidy_options = ''
    let g:ale_c_cppcheck_executable = 'cppcheck'
    let g:ale_c_cppcheck_options = '--enable=style'
    let g:ale_c_cpplint_executable = 'cpplint'
    let g:ale_c_cpplint_options = ''
    let g:ale_c_cquery_cache_directory = '/home/samuel/.cache/cquery'
    let g:ale_c_cquery_executable = 'cquery'
    let g:ale_c_flawfinder_error_severity = 6
    let g:ale_c_flawfinder_executable = 'flawfinder'
    let g:ale_c_flawfinder_minlevel = 1
    let g:ale_c_flawfinder_options = ''
    let g:ale_c_parse_compile_commands = 1
    let g:ale_c_parse_makefile = 0
     
     Global Variables:
    " Press Space to read :help for a setting
    let g:ale_cache_executable_check_failures = v:null
    let g:ale_change_sign_column_color = v:null
    let g:ale_command_wrapper = ''
    let g:ale_completion_delay = 100
    let g:ale_completion_enabled = 1
    let g:ale_completion_max_suggestions = 50
    let g:ale_disable_lsp = 'auto'
    let g:ale_echo_cursor = v:true
    let g:ale_echo_msg_error_str = 'Error'
    let g:ale_echo_msg_format = '%code: %%s'
    let g:ale_echo_msg_info_str = 'Info'
    let g:ale_echo_msg_warning_str = 'Warning'
    let g:ale_enabled = 1
    let g:ale_fix_on_save = v:false
    let g:ale_fixers = {}
    let g:ale_history_enabled = v:true
    let g:ale_info_default_mode = 'preview'
    let g:ale_history_log_output = v:true
    let g:ale_keep_list_window_open = 0
    let g:ale_lint_delay = 200
    let g:ale_lint_on_enter = v:true
    let g:ale_lint_on_filetype_changed = v:true
    let g:ale_lint_on_insert_leave = v:true
    let g:ale_lint_on_save = v:true
    let g:ale_lint_on_text_changed = 'normal'
    let g:ale_linter_aliases = {}
    let g:ale_linters = {}
    let g:ale_linters_explicit = v:false
    let g:ale_linters_ignore = {}
    let g:ale_list_vertical = v:false
    let g:ale_list_window_size = 10
    let g:ale_loclist_msg_format = '%code: %%s'
    let g:ale_max_buffer_history_size = 20
    let g:ale_max_signs = v:null
    let g:ale_maximum_file_size = v:null
    let g:ale_open_list = v:false
    let g:ale_pattern_options = v:null
    let g:ale_pattern_options_enabled = v:null
    let g:ale_root = {}
    let g:ale_set_balloons = v:false
    let g:ale_set_highlights = v:true
    let g:ale_set_loclist = v:true
    let g:ale_set_quickfix = v:false
    let g:ale_set_signs = v:true
    let g:ale_sign_column_always = v:null
    let g:ale_sign_error = v:null
    let g:ale_sign_info = v:null
    let g:ale_sign_offset = v:null
    let g:ale_sign_style_error = v:null
    let g:ale_sign_style_warning = v:null
    let g:ale_sign_warning = v:null
    let g:ale_sign_highlight_linenrs = v:null
    let g:ale_type_map = {}
    let g:ale_use_neovim_diagnostics_api = v:true
    let g:ale_use_global_executables = v:null
    let g:ale_virtualtext_cursor = 'all'
    let g:ale_warn_about_trailing_blank_lines = v:true
    let g:ale_warn_about_trailing_whitespace = v:true
     
      Command History:

    (executable check - failure) cspell
    (executable check - failure) flawfinder
    (finished - exit code 1) ['/bin/bash', '-c', '''clang'' -S -x c -o /dev/null -iquote ''/data/neues Zeug/rtes-synchronome-project/src/exe'' -O0 -Wall -Wextra -pedantic -fprofile-arcs -ftest-coverage -I ''/data/neues Zeug/rtes-synchronome-project/src'' -std=gnu23 -Wall -Wextra -pedantic - < ''/tmp/nvim.samuel/qDjNgH/16/capture.c''']

    <<<OUTPUT STARTS>>>
    In file included from <stdin>:1:
    In file included from /data/neues Zeug/rtes-synchronome-project/src/lib/camera.h:10:
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:16: error: expected identifier
       12 | typedef enum { false, true } bool;
          |                ^
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:23: error: expected identifier
       12 | typedef enum { false, true } bool;
          |                       ^
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:29: error: expected ';' after enum
       12 | typedef enum { false, true } bool;
          |                             ^
          |                             ;
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:1: warning: typedef requires a name [-Wmissing-declarations]
       12 | typedef enum { false, true } bool;
          | ^~~~~~~
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:30: warning: declaration does not declare anything [-Wmissing-declarations]
       12 | typedef enum { false, true } bool;
          |                              ^~~~
    2 warnings and 3 errors generated.
    <<<OUTPUT ENDS>>>

    (executable check - failure) ccls
    (finished - exit code 0) ['/bin/bash', '-c', '''clang-check'' -analyze ''/data/neues Zeug/rtes-synchronome-project/src/exe/capture.c'' -p ''/data/neues Zeug/rtes-synchronome-project''']

    <<<NO OUTPUT RETURNED>>>

    (finished - exit code 0) ['/bin/bash', '-c', '''clang-tidy'' ''/data/neues Zeug/rtes-synchronome-project/src/exe/capture.c'' -p ''/data/neues Zeug/rtes-synchronome-project''']

    <<<OUTPUT STARTS>>>
    src/exe/capture.c:130:2: warning: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
      130 |         CALLOC(data->rgb_buffer, data->rgb_buffer_size, 1);
          |         ^
    src/lib/global.h:27:3: note: expanded from macro 'CALLOC'
       27 |                 fprintf( stderr, "calloc failed! out of memory!" ); \
          |                 ^~~~~~~
    src/exe/capture.c:130:2: note: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11
      130 |         CALLOC(data->rgb_buffer, data->rgb_buffer_size, 1);
          |         ^
    src/lib/global.h:27:3: note: expanded from macro 'CALLOC'
       27 |                 fprintf( stderr, "calloc failed! out of memory!" ); \
          |                 ^~~~~~~
    src/exe/capture.c:179:4: warning: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
      179 |                         fprintf(stderr, "camera does not support expected format!\n");
          |                         ^~~~~~~
    src/exe/capture.c:179:4: note: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11
      179 |                         fprintf(stderr, "camera does not support expected format!\n");
          |                         ^~~~~~~
    <<<OUTPUT ENDS>>>

    (executable check - failure) cppcheck
    (executable check - failure) cpplint
    (executable check - failure) cquery
    (executable check - failure) cspell
    (executable check - failure) flawfinder
    (finished - exit code 1) ['/bin/bash', '-c', '''clang'' -S -x c -o /dev/null -iquote ''/data/neues Zeug/rtes-synchronome-project/src/exe'' -O0 -Wall -Wextra -pedantic -fprofile-arcs -ftest-coverage -I ''/data/neues Zeug/rtes-synchronome-project/src'' -std=gnu23 -Wall -Wextra -pedantic - < ''/tmp/nvim.samuel/qDjNgH/17/capture.c''']

    <<<OUTPUT STARTS>>>
    In file included from <stdin>:1:
    In file included from /data/neues Zeug/rtes-synchronome-project/src/lib/camera.h:10:
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:16: error: expected identifier
       12 | typedef enum { false, true } bool;
          |                ^
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:23: error: expected identifier
       12 | typedef enum { false, true } bool;
          |                       ^
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:29: error: expected ';' after enum
       12 | typedef enum { false, true } bool;
          |                             ^
          |                             ;
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:1: warning: typedef requires a name [-Wmissing-declarations]
       12 | typedef enum { false, true } bool;
          | ^~~~~~~
    /data/neues Zeug/rtes-synchronome-project/src/lib/global.h:12:30: warning: declaration does not declare anything [-Wmissing-declarations]
       12 | typedef enum { false, true } bool;
          |                              ^~~~
    2 warnings and 3 errors generated.
    <<<OUTPUT ENDS>>>

    (executable check - failure) ccls
    (finished - exit code 0) ['/bin/bash', '-c', '''clang-check'' -analyze ''/data/neues Zeug/rtes-synchronome-project/src/exe/capture.c'' -p ''/data/neues Zeug/rtes-synchronome-project''']

    <<<NO OUTPUT RETURNED>>>

    (finished - exit code 0) ['/bin/bash', '-c', '''clang-tidy'' ''/data/neues Zeug/rtes-synchronome-project/src/exe/capture.c'' -p ''/data/neues Zeug/rtes-synchronome-project''']

    <<<OUTPUT STARTS>>>
    src/exe/capture.c:130:2: warning: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
      130 |         CALLOC(data->rgb_buffer, data->rgb_buffer_size, 1);
          |         ^
    src/lib/global.h:27:3: note: expanded from macro 'CALLOC'
       27 |                 fprintf( stderr, "calloc failed! out of memory!" ); \
          |                 ^~~~~~~
    src/exe/capture.c:130:2: note: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11
      130 |         CALLOC(data->rgb_buffer, data->rgb_buffer_size, 1);
          |         ^
    src/lib/global.h:27:3: note: expanded from macro 'CALLOC'
       27 |                 fprintf( stderr, "calloc failed! out of memory!" ); \
          |                 ^~~~~~~
    src/exe/capture.c:179:4: warning: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11 [clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling]
      179 |                         fprintf(stderr, "camera does not support expected format!\n");
          |                         ^~~~~~~
    src/exe/capture.c:179:4: note: Call to function 'fprintf' is insecure as it does not provide security checks introduced in the C11 standard. Replace with analogous functions that support length arguments or provides boundary checks such as 'fprintf_s' in case of C11
      179 |                         fprintf(stderr, "camera does not support expected format!\n");
          |                         ^~~~~~~
    <<<OUTPUT ENDS>>>

    (executable check - failure) cppcheck
    (executable check - failure) cpplint
    (executable check - failure) cquery
    (executable check - failure) cspell
    (executable check - failure) flawfinder
