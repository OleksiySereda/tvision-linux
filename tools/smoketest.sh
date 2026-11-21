#!/bin/sh
# Starts every program in bin/ inside tmux, checks that it draws a screen
# and exits cleanly on Alt-X.  Run from the tvision/ directory.
#
# The docdemos are started in docdemos/, because several of them display
# their own source file.

root=$(pwd)
fail=0
for path in bin/hello bin/tvdemo bin/tvedit bin/tvforms bin/tvg17b bin/tvguid*; do
    prog=$(basename "$path")
    [ -x "$path" ] || continue
    case "$prog" in
        tvg*) dir=docdemos ;;
        *)    dir=bin ;;
    esac
    out=$(cd "$root/$dir" && sh "$root/tools/tmuxrun.sh" "$root/$path" 80 25 \
              sleep:0.5 shot keys:Escape keys:M-x sleep:0.8 shot 2>&1)
    drawn=$(printf '%s\n' "$out" | sed -n '4p' | tr -d ' ')
    if printf '%s\n' "$out" | grep -q "EXIT CODE 0" && [ -n "$drawn" ]; then
        echo "ok    $prog"
    else
        echo "FAIL  $prog"
        printf '%s\n' "$out" | head -8
        fail=1
    fi
done
exit $fail
