import os

rules_file = os.listdir("C:\\Users\\ilatei\\Desktop\\RLTuple\\rules_traces")

def main():
    for rule in rules_file:
        if rule.endswith("traces") or "_0_" not in rule:
            continue
        cmd_arg = " ".join(["C:\\Users\\ilatei\\Desktop\\RLTuple\\main.exe", "f=rules_traces/" + rule, 
                "a=throughput", "c=RLTuple,PSTSS,TupleMerge,PartitionSort,DynamicTuple",
                 "> C:\\Users\\ilatei\\Desktop\\RLTuple\\result\\" + rule])
        
        print(cmd_arg)
        ss = os.system(cmd_arg)


if __name__ == '__main__':
    main()