#!/bin/sh

FilesDir=~/riscv-tools/riscv-opcodes

print_help() {
    echo '--parse (-p) to parse into temp.h file'
    echo '--save (-s) copy riscv-opc.h to toolchain, a backup is made before'
    echo '--restore (-r) restore backup file' 
    echo '--open (-o) [arg] opens file set by arg: c, temp, h'
}

if [ -z $1 ]
then
    print_help

elif [ $1 = '--parse' ] || [ $1 = "-p" ]
then
    cat my-opcodes | $FilesDir/parse-opcodes -c > ./temp.h
    echo 'Check temp.h file, the first match and masks are the custom ones'
elif [ $1 = '--save' ] || [ $1 = "-s" ]
then
    cp $FilesDir/../riscv-gnu-toolchain/riscv-binutils-gdb/include/opcode/riscv-opc.h $FilesDir/../riscv-gnu-toolchain/riscv-binutils-gdb/include/opcode/riscv-opc.backup     
    echo "Python for the magic" 
    ./parse_populate.py ./my-opcodes ./temp.h $FilesDir/../riscv-gnu-toolchain/riscv-binutils-gdb/include/opcode/riscv-opc.h  
    echo 'Done'

elif [ $1 = '--restore' ] || [ $1 = "-r" ]
then
    mv $FilesDir/../riscv-gnu-toolchain/riscv-binutils-gdb/include/opcode/riscv-opc.backup $FilesDir/../riscv-gnu-toolchain/riscv-binutils-gdb/include/opcode/riscv-opc.h   
    echo 'Done'

elif [ $1 = '--open' ] || [ $1 = "-o" ]
then
    if [ -z $2 ]
    then
        print_help
    elif [ $2 = 'c' ]
    then
        vi $FilesDir/../riscv-gnu-toolchain/riscv-binutils-gdb/opcodes/riscv-opc.c  
    elif [ $2 = 'temp' ]
    then
        vi ./temp.h
    elif [ $2 = 'h' ]
    then
        vi $FilesDir/../riscv-gnu-toolchain/riscv-binutils-gdb/include/opcode/riscv-opc.h
    fi
else
    print_help
fi
