#!/usr/bin/env python3

#Imports
import argparse
from utils import make_utils as utils

def config_argparse(args = None):
    parser = argparse.ArgumentParser(description='Process opcodes files and insert them in the compiler.')
    excl_grp = parser.add_mutually_exclusive_group(required=True)
    excl_grp.add_argument('--build-templates', '-t', dest='execution_mode', action='store_const', const='build-templates', help='Expand templates from the ./templates dir. Create riscv-opc.c. Output is saved to ./build/templates.')
    excl_grp.add_argument('--parse', '-p', dest='execution_mode', action='store_const', const='parse', help='Parse the expanded templates into matches and masks (riscv-opc.h)')
    excl_grp.add_argument('--save', '-s', dest='execution_mode', action='store_const', const='save', help='Append matches and masks to riscv-opc.h and append contents of riscv-opc.c in the compilation toolchain. Backup is executed alongside.')
    excl_grp.add_argument('--build-opcodes', '-b', dest='execution_mode', action='store_const', const='build-opcodes', help='Equivalent to build-templates -> parse -> save.')
    excl_grp.add_argument('--restore_all', '-r', dest='execution_mode', action='store_const', const='restore-all', help='Restore all modified files from backups.')
    excl_grp.add_argument('--restore_delete', '-d', dest='execution_mode', action='store_const', const='restore-delete', help='Restore all modified files from backups and delete the backups.')
   
    return parser.parse_args(args)

if __name__ == "__main__":
    config = config_argparse()
    exm = config.execution_mode
    if exm == "build-templates":
        utils.expand_templates_to_local_and_create_local_riscv_opc_c()
    elif exm == "parse":
        utils.append_registers_to_parse_opcodes()
        utils.parse_expanded_insts_to_riscv_opc_h_local_file()
    elif exm == "save":
        utils.save_local_riscv_opc_h_file_to_toolchain()
        utils.append_insts_to_risc_opc_c_file_in_toolchain()
    elif exm == "build-opcodes":
        utils.append_registers_to_parse_opcodes()
        utils.expand_templates_to_local_and_create_local_riscv_opc_c()
        utils.append_insts_to_risc_opc_c_file_in_toolchain()
        utils.parse_expanded_insts_to_riscv_opc_h_local_file()
        utils.save_local_riscv_opc_h_file_to_toolchain()
    elif exm == "restore-all":
        utils.restore_files_from_backup()
    elif exm == "restore-delete":
        utils.restore_files_from_backup_and_delete_backups()
