#!/usr/bin/python3
import csv
import glob
import sys
import json
import re
import os

def getDataHeaders(fields_file): 
    #Data[Header]["filters"][Array]["name"] -> String
    #Data[Header]["filters"][Array]["value"] -> String
    #Data[Header]["type"] -> String
    Data = {}
    with open(fields_file, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        # next(reader)
        # Create lists from reader fieldnames
        types = next(reader)
        for field in reader._fieldnames:
            Data[field] = dict(type=types[field],filters=[])
        for row in reader:
            for field in Data.keys():
                if row[field] != "":
                    Data[field]["filters"].append({"name":row[field], "value":None})
    return Data

def getArgs():
    t_args = sys.argv[1].split("_")
    return t_args

def getTarget(target_file, type):
    #iter[string] / object (json)
    if type == "args":
        #Get Data from this program args
        return getArgs()
    t_path = list(glob.glob("./"+target_file))[0]
    with open(t_path, "r") as t_file:
        if type == "json":
            return json.load(t_file)
        return list(t_file)

def extract_args(headers_data, run_data):
    for header in headers_data:
        if header["name"] == "args.ARCH":
            header["value"] = run_data[0]
        if header["name"] == "args.PREFETCHER":
            header["value"] = run_data[1]
        if header["name"] == "args.N_ELEMENTS":
            header["value"] = run_data[2]
        if header["name"] == "args.N_ELEMENTS_START":
            header["value"] = run_data[3]
        if header["name"] == "args.ITER_COUNT":
            header["value"] = run_data[4]
    return headers_data

def extract_json(headers_data, run_data):
    for header in headers_data:
        o_path = header["name"].split(".")[1:] 
        access = run_data
        for o_dir in o_path:
            try:
                access = access[o_dir]
            except TypeError:
                access = access[int(o_dir)]
            except KeyError:
                header["value"]="None"
                continue
        header["value"] = access
    return headers_data

def extract_regexp(file_iter, pattern):
    # re_pattern = re.template(pattern)
    for istring in file_iter:
        imatch = re.match(pattern, istring)
        if imatch:
            return(imatch.groups()[0])

def extract_log(headers_data, run_data):
    for header in headers_data:
        if header["name"] == "log.Kernel_Time":
            header["value"] = extract_regexp(run_data, r"elapsed time, s:\s+(\d+.\d+)")
        if header["name"] == "log.CACHE_SIZE":
            header["value"] = extract_regexp(run_data, r"CACHE_SIZE\s+(\d+)")
        if header["name"] == "log.KERNEL_SIZE":
            header["value"] = extract_regexp(run_data, r"KERNEL_SIZE\s+(\d{1,2})$")
        if header["name"] == "log.N_ELEMENTS":
            header["value"] = extract_regexp(run_data, r"N_ELEMENTS\s+(\d+)$")
        if header["name"] == "log.N_ELEMENTS_START":
            header["value"] = extract_regexp(run_data, r"N_ELEMENTS_START\s+(\d+)$")
        if header["name"] == "log.ITER_COUNT":
            header["value"] = extract_regexp(run_data, r"ITER_COUNT\s+(\d+)$")
        if header["name"] == "log.Result":
            header["value"] = extract_regexp(run_data, r"Result:\s+(\d+.\d+)$")
        if header["name"] == "log.Last_Tick":
            header["value"] = extract_regexp(run_data, r"exiting with last active thread context\s+@\s+(\d+)")
    return headers_data

def extract_stats(headers_data, run_data):
    re_pattern = r"^(?P<header>\S+)\s+(?P<value>\d+[.]?\d*).+$"
    for line in run_data:
        if line.find("End Simulation Statistics") != -1:
            return headers_data
        imatch = re.match(re_pattern, line)
        if imatch:
            # print(imatch)
            for header in headers_data:
                if header["name"] == "stats." + imatch["header"]:
                    header["value"] = imatch["value"]
    # print(headers_data)
    return headers_data

def extract_data(headers_data, run_data, type):
    #headers_data[Array]["name"] -> String
    #headers_data[Array]["value"] -> String
    #run_data-> iter[Strings] / object (json)
    if type == "args":
        return extract_args(headers_data, run_data)
    elif type == "json":
        return extract_json(headers_data, run_data)
    elif type == "log":
        return extract_log(headers_data, run_data)
    elif type == "stats":
        return extract_stats(headers_data, run_data)
    else:
        raise TypeError("No type \"{}\" in parseable types\n".format(type))
    return headers_data

def print_data_to_file(data, results_filename):
    with open(results_filename, "w") as out_file:
        writer = csv.writer(out_file)
        csv_headers = []
        csv_data = []
        for item in data:
            csv_headers.append(item["name"])
            csv_data.append(item["value"])
        writer.writerow(csv_headers)
        writer.writerow(csv_data)

def main(fields_file, results_filename):
    Data_headers = getDataHeaders(fields_file) #Contains the headers that filter the data in Target_dict's files
    # print(Data_headers)
    Target_dict = {}
    for target in Data_headers.keys():
        Target_dict[target] = getTarget(target, Data_headers[target]["type"])
    # print(Target_dict)
    Data_final = []
    for target in Data_headers.keys():
        Data_final.extend(extract_data(Data_headers[target]["filters"], Target_dict[target], Data_headers[target]["type"]))
    print_data_to_file(Data_final, results_filename)


if __name__ == "__main__":
    thread_dir = sys.argv[2]
    os.chdir(thread_dir)
    main('ExtractTargets.csv',"m5out/data_extracted.csv")