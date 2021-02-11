#!/usr/bin/python3
from shutil import copy
from os import makedirs, path as ospath, symlink, getcwd

def docopy(src, dest):
    makedirs(ospath.dirname(dest), exist_ok=True)
    symlink(getcwd()+"/"+src, dest)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file", help="File to use as template")
    parser.add_argument("output_file", help="File name including path to save the render")
    args = parser.parse_args()
    docopy(args.input_file, args.output_file)
