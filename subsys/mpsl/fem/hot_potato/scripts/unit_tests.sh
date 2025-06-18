#!/bin/env bash

SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

MODE="run"
OPTION=""
RESULT=0

POSITIONAL_ARGS=()
while [[ $# -gt 0 ]]; do
    case $1 in
        --coverage) # generate coverage
            MODE="coverage"
            shift # past argument
            ;;
        --clean)
            MODE="clean"
            shift
            ;;
        -f|--fail-fast) # generate coverage
            FAIL_FAST=1
            shift # past argument
            ;;
        -*)
            echo "Unknown option $1"
            exit 1
            ;;
        *)
            POSITIONAL_ARGS+=("$1") # save positional arg
            shift # past argument
            ;;
    esac
done
set -- "${POSITIONAL_ARGS[@]}" # restore positional parameters

export OT_PATH="${OT_PATH:-"${SCRIPT_DIR}/../../../../../../modules/lib/openthread"}"

function fail()
{
    if [ -n "$FAIL_FAST" ]; then
        exit 1
    else
        RESULT=1
    fi
}

function run()
{
    ceedling test:all || fail
}

function coverage()
{
    ceedling gcov:all utils:gcov || fail
}

case "$MODE" in
    "run")
        run
        ;;
    "clean")
        ceedling clean
        ;;
    "coverage")
        coverage
        ;;
esac

exit $RESULT
