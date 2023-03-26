#!/bin/bash

bazel run //:format_bazel.test &&
bazel run //:format_python.test &&
bazel run //:lint_python.test &&
bazel run //:format_cc.test
