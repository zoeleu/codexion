#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
trace=$(mktemp)
trap 'rm -f "$trace"' EXIT

run_case() {
 local label=$1
 shift
 local coders=$1 quota=$6
 timeout 15 ./codexion "$@" > "$trace"
 awk -v n="$coders" -v quota="$quota" -v label="$label" '
  function fail(message) { print message > "/dev/stderr"; bad=1; exit 1 }
  $1 !~ /^[0-9]+$/ || $2 < 1 || $2 > n { fail("invalid log prefix") }
  ended { fail("message after burnout") }
  $1 < previous { fail("timestamps out of order") }
  { previous=$1 }
  /has taken a dongle$/ { dongles[$2]++; next }
  /is compiling$/ {
   left=($2==1 ? n : $2-1); right=($2==n ? 1 : $2+1)
   if (compiling[left] || compiling[right]) fail("adjacent compile overlap")
   if (dongles[$2]!=2) fail("compile without two acquisitions")
   dongles[$2]=0; compiling[$2]=1; started[$2]++; next
  }
  /is debugging$/ { compiling[$2]=0; completed[$2]++; next }
  /is refactoring$/ { next }
  /burned out$/ { ended=1; next }
  { fail("unknown message") }
  END {
   if (!bad && ended && label!="too_many") fail("unexpected burnout")
   if (!bad && !ended)
    for (i=1;i<=n;i++)
     if (started[i]!=quota || completed[i]!=quota) fail("quota incomplete")
   if (bad) exit 1
  }
 ' "$trace"
 printf 'PASS %s\n' "$label"
}

run_case basic_fifo 4 800 200 200 200 5 10 fifo
run_case basic_edf 4 800 200 200 200 5 10 edf
run_case success_fifo 10 10000 100 100 100 5 50 fifo
run_case large_edf 20 5000 500 500 500 10 100 edf
run_case low_cooldown 5 2000 100 100 100 20 1 fifo
run_case long_actions 3 4000 600 600 600 2 50 fifo
run_case zero_compile 5 1000 200 200 200 0 10 fifo
test ! -s "$trace"
run_case tight_timings 4 410 100 100 100 5 5 edf
run_case big 100 10000 66 24 87 10 10 fifo
run_case max_coders 300 10000 100 100 100 5 10 edf
run_case too_many 999 1000 200 200 200 5 50 fifo
