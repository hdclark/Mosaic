#!/bin/sh

cat "$@" | \
  tr '\n' '\0' | \
  sed -e 's/.*[<]soln[>]//' | \
  sed -e 's/[<]\/soln[>].*$//' | \
  sed -e 's/[<]img[>].*[<]\/img[>]//' | \
  tr '\0' '\n' | \
  sed -e '/^\s*$/d'



