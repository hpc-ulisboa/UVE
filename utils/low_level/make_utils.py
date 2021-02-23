import os
import subprocess
import shutil
from pathlib import Path
from low_level.rules_utils import parse_rules, subst_rules
from low_level.parse_populate import produce as parse_populate
from low_level.registers_utils import parse_registers, expand_registers
from low_level.files_utils import get_registers, get_rules, get_templates, get_toolchain_dir, get_parse_opcodes_script


def expand_templates_to_local_and_create_local_riscv_opc_c():
    template_files = get_templates()
    rules = get_rules()["rules"]
    registers = get_registers()["registers"]    
        
    print("Written templates to:")
        
    riscv_opc_c_file_strings = ""
    for template_file, template_out in zip(template_files["input_files"],template_files["output_files"]):
        (values_for_expanded, values_for_riscv_opc_c) = subst_rules(rules, registers, template_file.read_text())
        template_out.write_text(values_for_expanded)
        print("\t" + str(template_out))
        riscv_opc_c_file_strings += values_for_riscv_opc_c
    save_c_file_insts_to_local_riscv_opc_c_file(riscv_opc_c_file_strings)

def append_registers_to_parse_opcodes():
    registers = get_registers()["registers"]

    parse_opcodes_file = Path(".").resolve().glob('./ext_modules/riscv-opcodes/parse-opcodes')
    parse_opcodes_file = list(parse_opcodes_file)[0]
    parse_file_backup = parse_opcodes_file.with_suffix(".backup")
    file_contents = parse_opcodes_file.read_text()
    if not parse_file_backup.is_file(): 
        parse_file_backup.write_text(file_contents)
        print("Backed up parse-opcodes file, at: {}".format(parse_file_backup))
    
    expanded_registers_text = expand_registers(registers, file_contents)
    
    parse_opcodes_file.write_text(expanded_registers_text)
    print("Appended registers to parse-opcodes script file, at: {}".format(parse_opcodes_file))


def parse_expanded_insts_to_riscv_opc_h_local_file():
    cur_path = Path(".")
    opcodes_files = list(cur_path.glob('./build/templates/*.expanded'))
    opcodes = ""
    for file in opcodes_files:
        opcodes += file.read_text()
    
    subprocess.run(["chmod", "+x", str(get_parse_opcodes_script())])
    return_args = subprocess.run([str(get_parse_opcodes_script()), "-c"], input=bytes(opcodes,"UTF-8"), stdout=subprocess.PIPE)

    output_file = opcodes_files[0].parents[1] / "riscv-opc.h"
    result = output_file.write_bytes(return_args.stdout)
    if result != 0:
        print("Written parse result from parse-opcodes to local riscv-opc.h file, at: {}".format(output_file))

def save_local_riscv_opc_h_file_to_toolchain():
    cur_path = Path(".")
    origin_file = list(cur_path.glob('./build/riscv-opc.h'))[0]
    
    working_dir = get_toolchain_dir() / "riscv-binutils" / "include" / "opcode"
    backup_file = working_dir / "riscv-opc.h.backup"
    final_file = working_dir / "riscv-opc.h"
    
    if not backup_file.is_file():
        shutil.copy(final_file, backup_file)
        print("Backed up {} file into {}".format(final_file, backup_file))
    parse_populate(origin_file, final_file)
    print("Appended contents of local riscv-opc.h file to toolchain path, at: {}".format(final_file))


def save_c_file_insts_to_local_riscv_opc_c_file(insts_strings):
    riscv_opc_c_local_file = Path("./build") / "riscv-opc.c"
    riscv_opc_c_local_file.write_text(insts_strings)
    print("Created local riscv-opc.c file from expanded templates, at: {}".format(riscv_opc_c_local_file))

def append_insts_to_risc_opc_c_file_in_toolchain():
    insts_strings_file = list(Path(".").resolve().glob("./build/riscv-opc.c"))[0].read_text()
    riscv_opc_c_file = list(Path(".").resolve().glob("./ext_modules/riscv-gnu-toolchain/riscv-binutils/opcodes/riscv-opc.c"))[0]
    riscv_opc_c_file_backup = riscv_opc_c_file.with_name("riscv-opc.c.backup")
    contents = riscv_opc_c_file.read_text()

    if not riscv_opc_c_file_backup.is_file(): 
        riscv_opc_c_file_backup.write_text(contents)
        print("Backed up {} file into {}".format(riscv_opc_c_file, riscv_opc_c_file_backup))

    identification_string = "const struct riscv_opcode riscv_opcodes[] =\n{"
    replacement_string = identification_string + "\n" + insts_strings_file 
    replace_result = contents.replace(identification_string, replacement_string)
    riscv_opc_c_file.write_text(replace_result)
    print("Appended contents of local riscv-opc.c file to toolchain path, at: {}".format(riscv_opc_c_file))    

def restore_delete_files_from_backup(delete):
    backup_files = list(Path(".").resolve().glob("./ext_modules/**/*.backup"))
    print("Restored the following files from backup:")
    for backup_file in backup_files:
        restored_file = backup_file.with_suffix("")
        shutil.copy(backup_file,restored_file)
        if delete:
            backup_file.unlink()
        print(str(restored_file) + ("\t and deleted the backup file" if not delete else ""))

def restore_files_from_backup():
    restore_delete_files_from_backup(False)

def restore_files_from_backup_and_delete_backups():
    restore_delete_files_from_backup(True)  

if __name__ == "__main__":
    pass
    
