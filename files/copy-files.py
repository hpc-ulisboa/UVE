from pathlib import Path
import shutil

files = ["riscv-dis.c", "riscv-opc.c", "riscv.h", "tc-riscv.c"]
files_parent = ["opcodes", "opcodes", "opcode", "config"]

if __name__ == "__main__":
    base_level = Path(".").resolve()
    src_dir = list(base_level.glob("./files"))[0]
    
    print("The following files were copied to:")

    for file_name, file_parent in zip(files, files_parent):
        src_file = src_dir / file_name
        dest_file = list(base_level.glob("./ext_modules/riscv-gnu-toolchain/riscv-binutils/**/{}/{}"
            .format(file_parent, file_name)))[0]
        result = shutil.copy(src_file, dest_file)
        print(result)
