#!/bin/sh
# Drives a Turbo Vision program from a script file in a detached tmux
# session and checks what it shows: an end-to-end test for a program, or
# for an installer made with it.  Unlike tmuxrun.sh it waits for texts on
# the screen instead of fixed pauses, checks the exit code, and uses a
# tmux server of its own (the user's sessions are not touched).
#
#   tools/tvdrive.sh [-s COLSxROWS] [-t SECONDS] SCRIPT 'COMMAND'
#
#   -s   size of the terminal, 80x25 by default
#   -t   how long wait:, gone: and exit: wait at most, 10 s by default
#
# The script has one step per line ('#' starts a comment):
#
#   keys: Enter Tab M-x C-c F2   keys for tmux send-keys (Enter, Escape,
#                                Tab, BTab, Up, F1, M-x = Alt-X, C-x = Ctrl-X)
#   text: some text              typed as it is
#   wait: Step 2 of 3            waits until the screen shows the text
#   gone: Preparing              waits until it no longer does
#   sleep: 0.5                   a pause
#   shot                         prints the screen
#   resize: 100x30               changes the size of the terminal
#   exit: 0                      waits for the program to end, checks the code
#
# Exit status 0 and "tvdrive: ok" if every step passed; otherwise the step
# that failed, the screen at that moment and the program's stderr, and 1.
# Needs tmux.
#
# Example (installator/tests/smoke.keys):
#
#   wait: All steps finished.
#   keys: Enter
#   wait: Step 1 of 2: Text fields
#   keys: Enter
#   ...
#   exit: 0

size=80x25
timeout=10
while [ $# -gt 0 ]; do
    case "$1" in
        -s) size=$2; shift 2 ;;
        -t) timeout=$2; shift 2 ;;
        *)  break ;;
    esac
done
if [ $# -ne 2 ]; then
    echo "usage: $0 [-s COLSxROWS] [-t SECONDS] SCRIPT 'COMMAND'" >&2
    exit 2
fi
script=$1
command=$2
[ -r "$script" ] || { echo "tvdrive: cannot read $script" >&2; exit 2; }
command -v tmux >/dev/null 2>&1 || { echo "tvdrive: needs tmux" >&2; exit 2; }

sock=tvdrive-$$
tmp=$(mktemp -d "${TMPDIR:-/tmp}/tvdrive.XXXXXX")
cleanup() {
    tmux -L "$sock" kill-server 2>/dev/null
    rm -rf "$tmp"
}
trap cleanup EXIT INT TERM

export TERM=xterm-256color
tmux -L "$sock" -f /dev/null new-session -d -s tv -x "${size%x*}" -y "${size#*x}" \
    "$command 2>$tmp/stderr; echo \$? >$tmp/code; sleep 3600"

screen() {
    tmux -L "$sock" capture-pane -p -t tv 2>/dev/null
}

# waits until the screen has (want=1) or has not (want=0) the text
waitfor() {
    end=$(( $(date +%s) + timeout ))
    while :; do
        case "$(screen)" in
            *"$1"*) have=1 ;;
            *)      have=0 ;;
        esac
        [ $have = "$2" ] && return 0
        [ "$(date +%s)" -ge $end ] && return 1
        sleep 0.2
    done
}

fail() {
    echo "tvdrive: $script:$lineno: $1"
    echo "----- screen -----"
    screen
    echo "------------------"
    if [ -s "$tmp/stderr" ]; then
        echo "----- stderr -----"
        cat "$tmp/stderr"
    fi
    exit 1
}

lineno=0
while IFS= read -r line || [ -n "$line" ]; do
    lineno=$((lineno + 1))
    line=${line%"${line##*[![:space:]]}"}       # trailing blanks
    case "$line" in
        ''|'#'*) continue ;;
    esac
    step=${line%%:*}
    arg=${line#*:}
    [ "$step" = "$line" ] && arg=
    arg=${arg# }
    case "$step" in
        keys)   # several keys, as words
                # shellcheck disable=SC2086
                tmux -L "$sock" send-keys -t tv $arg; sleep 0.1 ;;
        text)   tmux -L "$sock" send-keys -t tv -l "$arg"; sleep 0.1 ;;
        wait)   waitfor "$arg" 1 || fail "'$arg' did not show within $timeout s" ;;
        gone)   waitfor "$arg" 0 || fail "'$arg' was still shown after $timeout s" ;;
        sleep)  sleep "$arg" ;;
        shot)   echo "----- screen ($script:$lineno) -----"; screen; echo "------------------" ;;
        resize) tmux -L "$sock" resize-window -t tv -x "${arg%x*}" -y "${arg#*x}"; sleep 0.3 ;;
        exit)   end=$(( $(date +%s) + timeout ))
                while [ ! -s "$tmp/code" ]; do
                    [ "$(date +%s)" -ge $end ] && fail "the program did not end within $timeout s"
                    sleep 0.2
                done
                code=$(cat "$tmp/code")
                [ "$code" = "$arg" ] || fail "exit code $code, expected $arg" ;;
        *)      fail "unknown step '$step'" ;;
    esac
done < "$script"
echo "tvdrive: ok"
