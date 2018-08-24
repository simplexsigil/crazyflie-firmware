import CF_functions as cff
import json
import argparse

import pandas as pd

def main():
    parser = argparse.ArgumentParser(
        description='Convert a binary logfile to json.')
    parser.add_argument('-i', '--input', metavar='infile',
                        help='The binary file that is converted to json.')

    parser.add_argument('-o', '--output', metavar='outfile', nargs='?',
                        help='The output json file.')

    args = parser.parse_args()

    in_path = args.input
    output = args.output if args.output is not None else args.input + ".json"

    logData = cff.decode(in_path)

    logData_list = {}

    for key in logData.keys():
        logData_list[key] = logData[key].tolist()

    f = open(output, "w")
    print("Number of lines written to {}: {}".format(
        output, str(f.write(json.dumps(logData_list)))))
    f.close()

    # Write to csv
    out_csv = args.input + ".csv"

    data_frame = pd.DataFrame.from_dict(logData)
    data_frame.to_csv(out_csv)

    print("Saved data to {}".format(out_csv))

    # Find missing ticks
    ticks = logData_list["tick"]

    min_tick = int(min(ticks))
    max_tick = int(max(ticks))

    step = int(ticks[1]) - int(ticks[0])

    print("first {}, last {}".format(min_tick, max_tick))
    missing = []
    t_set = set(ticks)
    for t in range(min_tick, max_tick+1, step):
        if not t in t_set:
            missing.append(t)

    ratio = len(missing) / len(t_set) 

    miss_file = args.input + "_mis.csv"
    df = pd.DataFrame.from_dict({"tick": missing, "ratio": ratio*100})
    df.to_csv(miss_file)

    print("Saved missing to {}".format(miss_file))    

    print("Loss ratio {:2.2f}% ".format(ratio*100))


if __name__ == "__main__":
    main()
