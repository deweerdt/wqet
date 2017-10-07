#!/bin/bash
exec $1 -sort-includes -i $(git ls-files | grep -E '.*\.[ch]' | grep -vE '^deps')
