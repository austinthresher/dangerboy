#!/bin/sh
ls -a tests/passed/*.gb | xargs -i bin/dangerboy {} -v
