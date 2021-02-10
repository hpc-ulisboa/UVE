from yaml import Loader, load
import re

def parse_registers(registers_payload):
    registers_obj = load(registers_payload, Loader=Loader)
    return registers_obj

def expand_registers(registers, string):
    template_line = "arglut['{}'] = ({},{})"
    re_line = r"arglut\['{}'\][\s\S]+?\n"
    output = ""
    for key, value in registers.items():
        arg = key
        range_hi = value["range"][0]
        range_lo = value["range"][1]
        string = re.sub(re_line.format(arg),"",string)
        output += template_line.format(arg, range_hi, range_lo) + "\n"
    
    match = r"(arglut\s+=\s+{}\n)"
    replace = r'\g<0>' + output 
    
    output = re.sub(match, replace, string) 
    return output
    
if __name__ == "__main__":
    from pathlib import Path
    import re

    input_registers_files = Path(".").resolve().parents[1].glob('**/*.registers')
    registers_files = {"registers_files" : list(input_registers_files)}
    assert(registers_files)
    registers = {}
    for file in registers_files["registers_files"]:
        registers.update(parse_registers(file.read_text()))
    
    parse_opcodes_file = Path(".").resolve().parents[1].glob('**/parse-opcodes')
    parse_opcodes_file = list(parse_opcodes_file)[0]
    parse_file_backup = parse_opcodes_file.parent / (parse_opcodes_file.stem + ".backup")
    file_contents = parse_opcodes_file.read_text()
    
    parse_file_backup.write_text(file_contents)
    
    expanded_registers_text = expand_registers(registers,
            file_contents)
    
    parse_opcodes_file.write_text(expanded_registers_text)


