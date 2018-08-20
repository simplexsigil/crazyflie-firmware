import CF_functions as cff
import json
import argparse

import pandas as pd

logfile = "testLog"


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

    # TODO: write to csv
    out_csv = args.input + ".csv"

    data_frame = pd.DataFrame.from_dict(logData)
    data_frame.to_csv(out_csv)

    print("Saved data to {}".format(out_csv))

if __name__ == "__main__":
    main()