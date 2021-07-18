#!/bin/bash

set -e

bazel build //test:test_message_def-docs
rm -r docs
cp -r --no-preserve=mode,ownership bazel-bin/test/test_message_def-docs_html docs
touch docs/.nojekyll
