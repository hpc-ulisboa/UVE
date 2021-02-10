import re

def removeComments(string):
    return re.sub(re.compile("#.*?\n|#.*" ) ,"" ,string) # remove all occurance singleline comments (#COMMENT\n ) from string

def removeBlankLines(string):
    return re.sub(re.compile('^[\s]*?\n',re.MULTILINE), "", string)

def blank_line(string):
    return True if len(string) == 0 else False


class TemplateSubst:
    def _internal_sub(self, string, key_name, key_value, subst_key):
        blank_kn = "." if key_name == "" else ""
        name_substitution_template = re.compile('{}<{}_k>'.format(blank_kn,subst_key))
        value_substitution_template = re.compile('<{}_v>'.format(subst_key))
        string = re.sub(name_substitution_template, key_name, string)
        string = re.sub(value_substitution_template, str(key_value), string)
        return string

    def _expand_keys_c(self, keys_to_expand, production_strings):
        if type(production_strings) is not list:
            production_strings = [production_strings]

        processing_key = ""
        try:
            processing_key = keys_to_expand.pop()
        except IndexError:
            return production_strings

        new_strings = []

        try:
            substitution_list = self.rules[processing_key]
            for substitution_dict in substitution_list:
                for (kn, kv) in substitution_dict.items():
                    for cur_string in production_strings:
                       new_strings.append(self._internal_sub(cur_string,
                                                                kn, kv,
                                                                processing_key))
        except KeyError as e:
            print('Warning: <{}_*> was not found in any rule'.format(processing_key))

        return self._expand_keys_c(keys_to_expand, new_strings)

    def _template_fill_h(self, expanded_string):
        re_name = r"^(\S+)"
        re_register = "|".join(list(self.registers))
        re_fp = r"^[\S]+(fp)"
        template_string = '{{"{name}", "{isa}", "{regs}", {match}, {mask}, match_opcode, 0}},'
        name_match = re.search(re_name, expanded_string).group(0)
        registers_matches = re.findall(re_register, expanded_string)
        fp_match = re.match(re_fp, expanded_string)
        regs = ""
        for match in registers_matches:
            if self.registers[match]["arg"] != "":
                #If instruction is FP, change arg 'd' to 'D'
                if self.registers[match]["arg"] == 'd' and fp_match is not None :
                    regs += "D" + ","
                else:
                    regs += self.registers[match]["arg"] + ","
        regs= regs[:-1]
        # regs = ",".join([self.registers[match]["arg"] for match in registers_matches])
        suffix = name_match.upper().replace(".","_")
        match = "MATCH_" + suffix
        mask = "MASK_" + suffix

        # JMNOTE: ISA is set to I currently to allow for no changes in gcc core
        return template_string.format(name=name_match, isa="I", regs=regs, match=match, mask=mask)

    def substitute_values(self, multiline_string):
        final_string_expanded = ""
        final_string_riscv_opc_c = ""
        re_pattern_c = re.compile('<([a-z]+?)_([a-z])>')

        for line in multiline_string.split('\n'):
            mid_string_expanded = ""
            keys = []
            matches_c = re.finditer(re_pattern_c, line)
            for match in matches_c:
                key = match.group(1)
                if key not in keys:
                    keys.append(key)

            if not blank_line(line):
                if not keys:
                    #Workaround for non templated strings
                    final_string_expanded += line + "\n"
                    final_string_riscv_opc_c += self._template_fill_h(line)+ "\n"
                else:
                    mid_string_expanded = self._expand_keys_c(keys.copy(), match.string)
                    final_string_expanded += "\n".join(mid_string_expanded) + "\n"
                    if mid_string_expanded:
                        for line in mid_string_expanded:
                            mid_string_c = self._template_fill_h(line)
                            final_string_riscv_opc_c += mid_string_c + "\n"

        return (final_string_expanded, final_string_riscv_opc_c)

    def __init__(self, rules, registers):
      self.rules = rules
      self.registers = registers

if __name__ == "__main__":
    pass