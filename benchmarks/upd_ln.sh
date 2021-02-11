#!/bin/bash
link=$1
# grab the target of the old link
target=$(readlink -- "$1")

# replace the first occurrence of oldusername with newusername in the target string
target=${target/$2/$3}

# Test the link creation
echo ln -s -- "$target" "$link"

if [ -n "$4" ]; then
    rm "$link"
    ln -s "$target" "$link"
    ls -la $link
else
    read -p "Replace? " -n 1 -r
    echo    # (optional) move to a new line
    if [[ ! $REPLY =~ ^[Yy]$ ]]
    then
        exit 1
    else
        rm "$link"
        ln -s "$target" "$link"
        ls -la $link
    fi
fi
