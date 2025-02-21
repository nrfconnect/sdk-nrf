#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from stats_nordic import StatsNordic

import argparse
import logging
import json


def main():
    parser = argparse.ArgumentParser(description="nRF Profiler event propagation statistics",
                                     allow_abbrev=False)
    parser.add_argument("dataset_name", help="Name of nRF Profiler dataset")
    parser.add_argument("test_presets", help="Test preset [*.json file]")
    parser.add_argument("--start_time", type=float, default=0.0,
                        help="Measurement start time [s]")
    parser.add_argument("--end_time", type=float, default=float('inf'),
                        help="Measurement end time [s]")
    parser.add_argument("--log", help="Log level")
    args = parser.parse_args()

    if args.log is not None:
        log_lvl_number = int(getattr(logging, args.log.upper(), None))
    else:
        log_lvl_number = logging.INFO

    try:
        with open(args.test_presets, "r") as test_preset_file:
            test_preset_dict = json.load(test_preset_file)
    except FileNotFoundError:
        print("File {} not found".format(args.test_presets))
        return
    except Exception as e:
        print("Exception while opening {}".format(args.test_presets))
        print(e)
        return

    sn = StatsNordic("{}.csv".format(args.dataset_name), "{}.json".format(args.dataset_name),
                     log_lvl_number)
    sn.calculate_stats(test_preset_dict, args.start_time, args.end_time)

if __name__ == "__main__":
    main()
