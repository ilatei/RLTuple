import random
import math
import os

tuples = ["nw_src", "nw_dst", "tp_src", "tp_dst", "nw_proto", 
        "dl_src", "dl_dst", "dl_vlan", "eth_type", "in_port", "nw_tos", "dl_vlan_pcp"]
ratio = {"in_port" : 0.02, "dl_src": 0.25, "dl_dst": 0.25, "eth_type" : 0.01, "dl_vlan": 0.2, "dl_vlan_pcp": 1, "nw_tos" : 1}
field_value = {"in_port" : [], "dl_src": [], "dl_dst": [], "eth_type" : [], "dl_vlan": [], "dl_vlan_pcp": [], "nw_tos" : []}

# max_len = {"nw_src" : 32, "nw_dst" : 32, "tp_src" : 16, "tp_dst" : 16, "nw_proto" : 8, 
#         "dl_src" : 48, "dl_dst" : 48, "dl_vlan" : 12, "eth_type" : 16, "in_port" : 32, "nw_tos" : 8, "dl_vlan_pcp" : 3}

max_len = {"nw_src" : 32, "nw_dst" : 32, "tp_src" : 16, "tp_dst" : 16, "nw_proto" : 8, 
        "dl_src" : 48, "dl_dst" : 48, "dl_vlan" : 12, "eth_type" : 16, "in_port" : 31, "nw_tos" : 8, "dl_vlan_pcp" : 3}
_dl_src = {'fa:16:3e': 838}
_dl_dst = {'01:80:c2': 1, '01:00:0c': 4, '00:e0:2b': 3, 'fa:16:3e': 12534, 'ff:ff:ff': 6, '01:00:00': 85, 'c2:81:09': 15, '00:00:00': 2}


rule_distributions = {
    5:{"nw_src" : 1, "nw_dst" : 1, "tp_src" : 1, "tp_dst" : 1, "nw_proto" : 1},
    6:{"nw_src" : 1, "nw_dst" : 1, "tp_src" : 1, "tp_dst" : 1, "nw_proto" : 1, "eth_type" : 0.3},
    7:{"nw_src" : 1, "nw_dst" : 1, "tp_src" : 1, "tp_dst" : 1, "nw_proto" : 1, "eth_type" : 0.3, "dl_src": 0.2},
    8:{"nw_src" : 1, "nw_dst" : 1, "tp_src" : 1, "tp_dst" : 1, "nw_proto" : 1, "eth_type" : 0.3, "dl_src": 0.2, "dl_dst": 0.8},
    9:{"nw_src" : 1, "nw_dst" : 1, "tp_src" : 1, "tp_dst" : 1, "nw_proto" : 1, "eth_type" : 0.3, "dl_src": 0.2, "dl_dst": 0.8, "in_port" : 0.1},
    10:{"nw_src" : 1, "nw_dst" : 1, "tp_src" : 1, "tp_dst" : 1, "nw_proto" : 1, "eth_type" : 0.3, "dl_src": 0.2, "dl_dst": 0.8, "in_port" : 0.1, "dl_vlan": 0.5},
    11:{"nw_src" : 1, "nw_dst" : 1, "tp_src" : 1, "tp_dst" : 1, "nw_proto" : 1, "eth_type" : 0.3, "dl_src": 0.2, "dl_dst": 0.8, "in_port" : 0.1, "dl_vlan": 0.5, "dl_vlan_pcp": 0.05},
    12:{"nw_src" : 1, "nw_dst" : 1, "tp_src" : 1, "tp_dst" : 1, "nw_proto" : 1, "eth_type" : 0.3, "dl_src": 0.2, "dl_dst": 0.8, "in_port" : 0.1, "dl_vlan": 0.5, "dl_vlan_pcp": 0.05, "nw_tos" : 0.05}}

# fields_num = {"5","6(in_port)",7,8,9,10,11,12}
# {"dl_dst" : 15734 / 16431; 12470 / 13778}
# {"dl_src" : 235 / 16431; 603}
# {"in_port" : 162 / 16431; 142}
# {"eth_type" : 4774/ 16431; 9509}
def random_mac(mac):
    mac_split = mac.split(":")
    while mac_split.__len__() < 6:
        mac_split.append(hex(random.randint(0,255))[2:])
    return ":".join(mac_split)

def random_with_weight(data_dict):
    sum_wt = sum(data_dict.values()) 
    ra_wt = random.uniform(0, sum_wt)
    cur_wt = 0
    for key in data_dict.keys():
        cur_wt += data_dict[key]
        if ra_wt <= cur_wt:
            return key

def generateItem(rule_num):
    for _field in field_value.keys():
        if _field == "dl_src":
            while field_value[_field].__len__() < ratio[_field] * rule_num:
                _mac = random_mac(random_with_weight(_dl_src))
                field_value[_field].append(_mac)
        elif _field == "dl_dst":
            while field_value[_field].__len__() < ratio[_field] * rule_num:
                _mac = random_mac(random_with_weight(_dl_dst))
                field_value[_field].append(_mac)
        elif _field == "eth_type":
            while field_value[_field].__len__() < ratio[_field] * rule_num:
                field_value[_field].append(hex(random.randint(0, pow(2, max_len[_field]) - 1)))
        else:
            while field_value[_field].__len__() < ratio[_field] * rule_num:
                field_value[_field].append(random.randint(0, pow(2, max_len[_field]) - 1))


def generate(in_file, out_file, field_num):
    fd = open(in_file, "r")
    ff = open(out_file, "w")
    rule_distribution = rule_distributions[field_num]

    for line in fd:
        outline = ""
        infos = line.split('\t')
        nw_src = infos[0][1:]
        if nw_src.split("/")[-1] != '0' and random.random() < rule_distribution["nw_src"]:
            outline += "nw_src=" + nw_src
            sflag = 1

        nw_dst = infos[1]
        if nw_dst.split("/")[-1] != '0' and random.random() < rule_distribution["nw_dst"]:
            outline += ", " + "nw_dst=" + nw_dst

        tp_src = infos[2].replace(" ","")
        tp_srcs = tp_src.split(":")
        if (tp_srcs[0] != '0' or tp_srcs[1] != '65535') and random.random() < rule_distribution["tp_src"]:
            outline += ", " + "tp_src=" + tp_srcs[0] + ":" + tp_srcs[1]
    
        tp_dst = infos[3].replace(" ","").split(":")
        if (tp_dst[0] != '0' or tp_dst[1] != '65535') and random.random() < rule_distribution["tp_dst"]:
            outline += ", " + "tp_dst=" + tp_dst[0] + ":" + tp_dst[1]

        nw_proto = infos[4].split("/")
        if nw_proto[1] != '0x00' and random.random() < rule_distribution["nw_proto"]:
            outline += ", " + "nw_proto=" + str(int(nw_proto[0], 16))
    
        if rule_distribution.__len__() != 0:
            for item in rule_distribution.keys():
                if item in ["nw_src", "nw_dst", "tp_src", "tp_dst", "nw_proto"]:
                    continue
                if random.random() < rule_distribution[item]:
                    outline += ", " + item + "=" + str(field_value[item][random.randint(0, field_value[item].__len__() - 1)])

        if outline.startswith(", "):
            ff.write(outline[2:] + "\n")
        elif outline.__len__() != 0:
            ff.write(outline + "\n")

fields_num = {5,6,7,8,9,10,11,12}
rule_numbers = {1000, 10000, 100000}
seeds = os.listdir("/home/lanan/classbench-master/parameter_files")

for rule_num in rule_numbers:
    generateItem(rule_num)
    # print(field_value)
    for seed in seeds:
        parameter_file = "../parameter_files/" + seed
        cmd = ["../db_generator/db_generator", "-bc", parameter_file , str(rule_num), "2", "-0.5", "0.1", "temp"]
        # print(" ".join(cmd))
        os.system(" ".join(cmd))
        for field_num in fields_num:
            out_file_name = "_".join([seed, str(rule_num), str(field_num)])
            generate("temp", "classifier/" + out_file_name, field_num)
