#!/usr/bin/python3
from os import makedirs, path as ospath
from jinja2 import Template, FileSystemLoader, Environment

def setup():
    t_loader = FileSystemLoader(searchpath="./")
    t_env = Environment(loader=t_loader)
    return t_env

def template(environment, filename, **kwargs):
    t_temp = environment.get_template(filename)
    return t_temp.render(kwargs)

def save_to_file(path, data):
    makedirs(ospath.dirname(path), exist_ok=True)
    with open(path, "w") as fh:
        fh.write(data)

def list_to_dict(li):
    di = dict()
    for item in li:
        sli = item.split("=")
        di.update({sli[0]:sli[1]})
    return di

def render(template_file, output_file, **kwargs):
    env = setup()
    templated = template(env, template_file, **kwargs)
    save_to_file(output_file,templated)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("template_file", help="File to use as template")
    parser.add_argument("output_file", help="File name including path to save the render")
    parser.add_argument('vars', nargs='*')
    args = parser.parse_args()
    # print(args)
    vars_dict = list_to_dict(args.vars)
    # print(vars_dict)
    render(args.template_file, args.output_file, **vars_dict)