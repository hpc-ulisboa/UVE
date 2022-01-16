# UVE Benchmarks

This repo contains a set of UVE benchmarks for use in developing, testing, and evaluation.
The UVE Reference card is also available in the pdf format.

Each benchmark must be put in the benchmarks folder, to automate benchmark creation there is a tool in ./tools/create_benchmark.sh that populates all the necessary files and customizes files based on provided arguments.
Global configurations can be changed by creating a global_configs.sh file in the root folder. Copy the global_configs_template.sh from ./tools.

An environment.source file is provided as a template, this file must be updated to reflect the target directory structure.
This file sourcing must result in the exporting of the following variables:
- `GEM5_RV_ROOT`: root path of the RV+UVE gem5 source
- `GEM5_RV`: path of the RV+UVE gem5 binary
- `RISCV`: must be set to the RV+UVE compiler path (<compiler_dir>)
- Also, the RV+UVE compiler bin path (<compiler_dir>/bin) must be added to the PATH variable.

Optionally, if you want to run benchmarks with ARM or SVE simulation:
- `GEM5_ARM_ROOT`: root path of the ARM+SVE gem5 source
- `GEM5_ARM`: path of the ARM+SVE gem5 binary
- `ARM+SVE` compiler bin path (<compiler_dir>/bin) must be added to the PATH variable.
