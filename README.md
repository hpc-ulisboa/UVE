# UVE - Compiler

The Unlimited Vector Extension Toolchain based on GCC. An automatic build system based on configuration files allows for automatic modification, compilation and installation of UVE instructions on the GCC toolchain.

## Setup
Run in the following order:

> git submodule --init --recursive
Fetches submodules

> ./set_repos.sh
Initializes repositories and applies patches

> ./inject_uve.sh
Injects UVE templated changes into code

> ./configure.sh
Configures build, builds toolchain and installs.
The toolchain installation path defaults to ../build/uve_tc

## Workflow
Run:
> ./rebuild.sh
Answer y after verifying that everything went ok.
This will build the new changes and updates GNU as (assembler) build

You can force a make clean of GNU as, by executing:
> ./rebuild.sh 1
