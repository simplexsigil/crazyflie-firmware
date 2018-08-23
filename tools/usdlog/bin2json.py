import CF_functions as cff
import json
import argparse

logfile = "testLog"


def main():
    parser = argparse.ArgumentParser(description='Convert a binary logfile to json.')
    parser.add_argument('in_path', metavar='infile',
                        help='The binary file that is converted to json.')

    parser.add_argument('out_path', metavar='outfile', nargs='?',
                        help='The output json file.')

    args = parser.parse_args()

    in_path = args.in_path
    out_path = args.out_path if args.out_path is not None else args.in_path + ".json"

    logData = cff.decode(in_path)

    for key in logData.keys():
        logData[key] = logData[key].tolist()

    f = open(out_path, "w")
    print("Number of lines written to {}: {}".format(out_path, str(f.write(json.dumps(logData)))))
    f.close()


if __name__ == "__main__":
    main()
