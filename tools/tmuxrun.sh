#!/bin/sh
# Drives a Turbo Vision program inside a detached tmux session and prints
# screen captures; used to test the port without a real terminal.
#
#   tools/tmuxrun.sh <program> <cols> <rows> [step ...]
#
# Each step is one of:
#   keys:<tmux send-keys arguments>   e.g. keys:M-x   keys:F10   keys:Enter
#   text:<literal text>
#   sleep:<seconds>
#   resize:<cols>x<rows>
#   shot                              print the screen
#   mouse:<x>,<y>                     left click at 0-based cell x,y

prog=$1; cols=$2; rows=$3; shift 3
export TERM=xterm-256color
tmux kill-server 2>/dev/null
tmux new-session -d -s tv -x "$cols" -y "$rows" \
    "$prog 2>/tmp/tv-stderr.txt; echo \"EXIT CODE \$?\"; sleep 30"
sleep 1

for step in "$@"; do
    case "$step" in
        keys:*)   tmux send-keys -t tv ${step#keys:}; sleep 0.4 ;;
        text:*)   tmux send-keys -t tv -l "${step#text:}"; sleep 0.4 ;;
        sleep:*)  sleep "${step#sleep:}" ;;
        resize:*) dim=${step#resize:}
                  tmux resize-window -t tv -x "${dim%x*}" -y "${dim#*x}"
                  sleep 0.6 ;;
        mouse:*)  xy=${step#mouse:}; x=$(( ${xy%,*} + 1 )); y=$(( ${xy#*,} + 1 ))
                  tmux send-keys -t tv -l "$(printf '\033[<0;%d;%dM\033[<0;%d;%dm' $x $y $x $y)"
                  sleep 0.4 ;;
        shot)     echo "----- screen -----"
                  tmux capture-pane -p -t tv
                  echo "------------------" ;;
    esac
done
tmux kill-server 2>/dev/null
cat /tmp/tv-stderr.txt
