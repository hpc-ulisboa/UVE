from pathlib import Path
from low_level.rules_utils import parse_rules
from low_level.registers_utils import parse_registers

def get_files(file_types):
    if file_types == "templates":
        templates_path = Path(".")
        input_template_files = templates_path.glob('**/templates/*.template')

        input_template_files = list(input_template_files)

        output_template_dir = (input_template_files[0].parents[1] / "build" / "templates")

        output_template_dir.mkdir(parents=True, exist_ok=True)

        output_template_files = []

        for file in input_template_files:
            output_template_files.append(output_template_dir / (file.stem + ".expanded"))
        
        return {"input_files": input_template_files, "output_files": output_template_files}
    
    elif file_types == "rules":
        rules_files = Path(".").glob('**/templates/*.rules')
        assert(rules_files)
        rules = {}
        for rules_file in rules_files:
            rules.update(parse_rules(rules_file.read_text()))
        return {"rules": rules}
    
    elif file_types == "registers":
        input_registers_files = Path(".").resolve().parents[1].glob('**/*.registers')
        registers_files = {"registers_files" : list(input_registers_files)}
        assert(registers_files)
        registers = {}
        for file in registers_files["registers_files"]:
            registers.update(parse_registers(file.read_text()))
        return {"registers": registers}
    else: 
        raise TypeError("Wrong Arguments Given")


def get_templates():
    return get_files("templates")

def get_rules():
    return get_files("rules")

def get_registers():
    return get_files("registers")

def get_directory(glob_string):
    return list(Path(".").resolve().glob(glob_string))[0]

def get_parse_opcodes_script():
    return get_directory("**/parse-opcodes")

def get_toolchain_dir():
    return get_directory("**/riscv-gnu-toolchain")
