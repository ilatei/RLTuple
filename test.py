import os

rules_file = os.listdir("C:\\Users\\ilatei\\Desktop\\RLTuple\\rules_traces")

def main():
    for rule in rules_file:
        if rule.endswith("traces") or "_1000_" not in rule:
            continue
        cmd_arg = " ".join(["C:\\Users\\ilatei\\Desktop\\RLTuple\\main.exe", "f=rules_traces/" + rule, 
                "a=throughput","d=yes", "c=PSTSS,TupleMerge,PartitionSort,DynamicTuple,RLTuple", "d=3", "t=100000", "to=0.8",
                 "> C:\\Users\\ilatei\\Desktop\\RLTuple\\result\\" + rule + "_throughput_0.8"])
        
        print(cmd_arg)
        ss = os.system(cmd_arg)

# "p=rules_traces/" + rule + "_traces", "to=0.0"
if __name__ == '__main__':
    main()