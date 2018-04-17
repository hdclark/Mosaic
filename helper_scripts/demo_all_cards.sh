#!/bin/sh

# Show a sampling of cards and record the results in a fleeting DB.
tmp_db=$(mktemp)
find ~/Flashcards/ -iname '*fcard' \
    -exec mosaic -d "${tmp_db}" -S ~/Flashcards/ '{}' \+
rm "${tmp_db}"

