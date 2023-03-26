#!/bin/bash

if [[ -n $(git ls-files --other --exclude-standard) ||
      -n $(git diff) ]]; then
  echo "ERROR: Unstaged changes in repo. Stage all changes before running fix_all.sh"
  exit 1
fi

bazel run //:format_bazel.fix &&
bazel run //:format_python.fix &&
bazel run //:format_cc.fix
