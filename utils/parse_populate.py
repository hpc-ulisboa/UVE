#!/usr/bin/env python3
import re
from pathlib import Path


def get_match_and_mask(file):
    string = file.read_text()
    match_mask_pattern = re.compile("#define\s+?(?:MATCH|MASK)_[A-z0-9]+?\s+?[0-z]+?\n")
    return re.findall(match_mask_pattern, string)
    

def append_new_insts(matches_and_masks, final_file):
    output = final_file.read_text()
    pattern = re.compile("(#define\s+RISCV_ENCODING_H.*\n)")
    final_file.write_text(re.sub(pattern,"\g<1>{}".format("\n".join(matches_and_masks)),output))

def produce(masks_file, final_file):

    append_new_insts(get_match_and_mask(masks_file),final_file)

    





