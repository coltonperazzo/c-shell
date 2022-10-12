#!/bin/bash

# Safer execution
# -e: exit immediately if a command fails
# -E: Safer -e option for traps
# -u: fail if a variable is used unset
# -o pipefail: exit immediately if command in a pipe fails
#set -eEuo pipefail
# -x: print each command before executing (great for debugging)
#set -x

# Convenient values
SCRIPT_NAME=$(basename $BASH_SOURCE)

# Default program values
SSHELL_EXEC="${PWD}/sshell"
TEST_CASE="all"

#
# Logging helpers
#
log() {
    echo -e "${*}"
}
info() {
    log "Info: ${*}"
}
warning() {
    log "Warning: ${*}"
}
error() {
    log "Error: ${*}"
}
die() {
    error "${*}"
    exit 1
}

#
# Line comparison
#
select_line() {
    # 1: string
    # 2: line to select
    echo "$(echo "${1}" | sed "${2}q;d")"
}

fail() {
    # 1: got
    # 2: expected
    log "Fail: got '${1}' but expected '${2}'"
}

pass() {
    # got
    log "Pass: ${1}"
}

compare_lines() {
    # 1: output
    # 2: expected
    # 3: score (output)
    declare -a output_lines=("${!1}")
    declare -a expect_lines=("${!2}")
    local __score=$3
    local partial="0"

    # Amount of partial credit for each correct output line
    local step=$(bc -l <<< "1.0 / ${#expect_lines[@]}")

    # Compare lines, two by two
    for i in ${!output_lines[*]}; do
        if [[ "${output_lines[${i}]}" == "${expect_lines[${i}]}" ]]; then
            pass "${output_lines[${i}]}"
            partial=$(bc <<< "${partial} + ${step}")
        else
            fail "${output_lines[${i}]}" "${expect_lines[${i}]}" ]]
        fi
    done

    # Return final score
    eval ${__score}="'${partial}'"
}

#
# Run generic test case
#
run_test_case () {
    # 1: multi-line string to feed to the shell
    # 2: (optional) strace the whole test
    local cmdline="${1}"
    local strace=false

    [[ $# -ge 2 ]] && strace="${2}"

    # These are global variables after the test has run so clear them out now
    unset STDOUT STDERR RET

    # Create temp files for getting stdout and stderr
    local outfile=$(mktemp)
    local errfile=$(mktemp)

    if [[ "${strace}" ==  true ]]; then
        strace -f \
            bash -c "echo -e \"${cmdline}\" | \
            ${SSHELL_EXEC}" >${outfile} 2>${errfile}
    else
        bash -c "echo -e \"${cmdline}\" | \
            ${SSHELL_EXEC}" >${outfile} 2>${errfile}
    fi

    # Get the return status, stdout and stderr of the test case
    RET="${?}"
    STDOUT=$(cat "${outfile}")
    STDERR=$(cat "${errfile}")

    # Clean up temp files
    rm -f "${outfile}"
    rm -f "${errfile}"
}

#
# Test cases
#
TEST_CASES=()

## Exit needs to exist no matter what!
builtin_exit() {
    log "--- Running test case: ${FUNCNAME} ---"
    run_test_case "exit\n"

    local line_array=()
    line_array+=("$(select_line "${STDERR}" "1")")
    line_array+=("$(select_line "${STDERR}" "2")")
    local corr_array=()
    corr_array+=("Bye...")
    corr_array+=("+ completed 'exit' [0]")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "${score}"
}
TEST_CASES+=("builtin_exit")

## Commands with no arguments
cmd_no_arg() {
    log "--- Running test case: ${FUNCNAME} ---"
    touch titi toto
    run_test_case "ls\nexit\n"
    rm titi toto

    local line_array=()
    line_array+=("$(select_line "${STDOUT}" "2")")
    line_array+=("$(select_line "${STDOUT}" "3")")
    local corr_array=()
    corr_array+=("titi")
    corr_array+=("toto")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "${score}"
}
TEST_CASES+=("cmd_no_arg")

## Commands with arguments
cmd_one_arg() {
    log "--- Running test case: ${FUNCNAME} ---"
    mkdir dir_test && touch dir_test/lstest
    run_test_case "ls dir_test\nexit\n"
    rm -rf dir_test

    local line_array=()
    line_array+=("$(select_line "${STDOUT}" "2")")
    local corr_array=()
    corr_array+=("lstest")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "${score}"
}
TEST_CASES+=("cmd_one_arg")

## Builtin commands
cd_pwd() {
    log "--- Running test case: ${FUNCNAME} ---"
    run_test_case "mkdir -p dir_test\ncd dir_test\npwd\nexit\n"
    rm -rf dir_test

    local line_array=()
    line_array+=("$(select_line "${STDOUT}" "4")")
    line_array+=("$(select_line "${STDERR}" "2")")
    local corr_array=()
    corr_array+=("${PWD}/dir_test")
    corr_array+=("+ completed 'cd dir_test' [0]")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "${score}"
}
TEST_CASES+=("cd_pwd")

## Output redirection
out_redir() {
    log "--- Running test case: ${FUNCNAME} ---"
    run_test_case "echo hello > t\ncat t\nexit\n"
    rm -f t

    local line_array=()
    line_array+=("$(select_line "${STDOUT}" "3")")
    local corr_array=()
    corr_array+=("hello")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "${score}"
}
TEST_CASES+=("out_redir")

## Piped commands
pipe() {
    log "--- Running test case: ${FUNCNAME} ---"
    echo -e "HELLO world\nhello WORLD" > t
    run_test_case "cat t | grep hello\nexit\n"
    rm -f t

    local line_array=()
    line_array+=("$(select_line "${STDOUT}" "2")")
    local corr_array=()
    corr_array+=("hello WORLD")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "${score}"
}
TEST_CASES+=("pipe")

## Extra feature #1: stdin redirection
stdin_redir() {
    log "--- Running test case: ${FUNCNAME} ---"
    echo hello > t
    run_test_case "grep he < t\nexit\n"
    rm -f t

    local line_array=()
    line_array+=("$(select_line "${STDOUT}" "2")")
    local corr_array=()
    corr_array+=("hello")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "${score}"
}
TEST_CASES+=("stdin_redir")

## Extra feature #2: remembered directories
dirs_simple() {
    log "--- Running test case: ${FUNCNAME} ---"
    local tmpdir=$(basename $(mktemp -d --tmpdir=.))
    run_test_case "pwd\npushd ${tmpdir}\ndirs\npopd\npwd\nexit\n"
    rm -rf "${tmpdir}"

    local line_array=()
    line_array+=("$(select_line "${STDOUT}" "2")")
    line_array+=("$(select_line "${STDOUT}" "5")")
    line_array+=("$(select_line "${STDOUT}" "6")")
    line_array+=("$(select_line "${STDOUT}" "9")")
    local corr_array=()
    corr_array+=("${PWD}")
    corr_array+=("${PWD}/${tmpdir}")
    corr_array+=("${PWD}")
    corr_array+=("${PWD}")

    local score
    compare_lines line_array[@] corr_array[@] score
    log "${score}"
}
TEST_CASES+=("dirs_simple")

#
# Main functions
#
parse_argvs() {
    local OPTIND opt

    while getopts "h?s:t:" opt; do
        case "$opt" in
        h|\?)
            echo "${SCRIPT_NAME}: [-s <sshell_path>] [-t <test_case>]" 1>&2
            exit 0
            ;;
        s)  SSHELL_EXEC="$(readlink -f ${OPTARG})"
            ;;
        t)  TEST_CASE="${OPTARG}"
            ;;
        esac
    done
}

check_vals() {
    # Check sshell path
    [[ -x ${SSHELL_EXEC} ]] || \
        die "Cannot find executable '${SSHELL_EXEC}'"

    # Check test case
    [[ " ${TEST_CASES[@]} all " =~ " ${TEST_CASE} " ]] || \
        die "Cannot find test case '${TEST_CASE}'"
}

grade() {
    # Make testing directory
    local tmpdir=$(mktemp -d --tmpdir=.)
    cd ${tmpdir}

    # Run test case(s)
    if [[ "${TEST_CASE}" == "all" ]]; then
        # Run all test cases
        for t in "${TEST_CASES[@]}"; do
            ${t}
            log "\n"
        done
    else
        # Run specific test case
        ${TEST_CASE}
    fi

    # Cleanup testing directory
    cd ..
    rm -rf "${tmpdir}"
}

parse_argvs "$@"
check_vals
grade
