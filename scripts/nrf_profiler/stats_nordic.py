#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging
import os
from enum import StrEnum

import matplotlib.pyplot as plt
import numpy as np
from processed_events import ProcessedEvents

OUTPUT_DIR_BASE = "data_stats"

class EventState(StrEnum):
    SUBMIT = "submit"
    PROCESSING_START = "processing_start"
    PROCESSING_END = "processing_end"

class StatsNordic:
    def __init__(self, events_filename, events_types_filename, log_lvl):
        self.dataset_name = events_filename.split(".")[0]
        self.processed_data = ProcessedEvents()
        self.processed_data.read_data_from_files(events_filename, events_types_filename)

        self.logger = logging.getLogger("stats_nordic")
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(log_lvl)
        self.log_format = logging.Formatter("[%(levelname)s] %(name)s: %(message)s")
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)

    def _get_timestamps(self, event_name, event_state, start_meas=0.0, end_meas=float("inf")):
        event_type_id = self.processed_data.get_event_type_id(event_name)
        if event_type_id is None:
            raise ValueError(f"Event not found in dataset: {event_name}")

        if not isinstance(event_state, EventState):
            raise ValueError(f"Invalid EventState: {event_state}")

        if event_state != EventState.SUBMIT:
            if not self.processed_data.is_event_tracked(event_type_id):
                raise ValueError(f"Event not tracked: {event_name}")

        trackings = list(filter(lambda x: x.submit.type_id == event_type_id,
                                self.processed_data.tracked_events))

        if event_state == EventState.SUBMIT:
            timestamps = np.fromiter(map(lambda x: x.submit.timestamp, trackings),
                                     dtype=float)
        elif event_state == EventState.PROCESSING_START:
            timestamps = np.fromiter(map(lambda x: x.proc_start_time, trackings),
                                     dtype=float)
        elif event_state == EventState.PROCESSING_END:
            timestamps = np.fromiter(map(lambda x: x.proc_end_time, trackings),
                                     dtype=float)
        else:
            raise ValueError(f"Invalid EventState: {event_state}")

        timestamps = timestamps[np.nonzero((timestamps > start_meas) & (timestamps < end_meas))]

        return timestamps

    def _calculate_times_between_ms(self, start_times, end_times):
        # Number of start times must match the number of end times.
        # Remove start/end time records to also ensure that start times are before end times.
        while end_times[0] <= start_times[0]:
            self.logger.warning("End time[0] <= start time[0], dropping the first end time")
            end_times = end_times[1:]

        while len(start_times) > len(end_times):
            self.logger.warning("Start/end times length mismatch, dropping the last start time")
            start_times = start_times[:-1]

        # Convert results to milliseconds
        times_between = (end_times - start_times) * 1000
        return times_between, start_times, end_times

    @staticmethod
    def _get_outlier_filter_mask(measurements, std_factor=3):
        mean = np.mean(measurements)
        std = np.std(measurements)
        limit_min = mean - std_factor * std
        limit_max = mean + std_factor * std

        return np.nonzero((measurements > limit_min) & (measurements < limit_max))

    @staticmethod
    def _times_between_to_stats_txt(preset_desc, output_dir, times_between_ms, out_filename="stats",
                                    logger=None):
        stats = {
            "Max" : f"{max(times_between_ms):.3f} ms",
            "Min" : f"{min(times_between_ms):.3f} ms",
            "Mean" : f"{np.mean(times_between_ms):.3f} ms",
            "Std dev" : f"{np.std(times_between_ms):.3f} ms",
            "Median" : f"{np.median(times_between_ms):.3f} ms",
            "Number of events" : str(len(times_between_ms))
        }

        key_len_max = max(len(k) for k in stats)
        val_len_max = max(len(v) for v in stats.values())

        out_filepath = os.path.join(output_dir, f"{out_filename}.txt")

        with open(out_filepath, "w") as f:
            f.write(f"Event propagation times statistics - {preset_desc}\n")
            for s in stats:
                f.write(f"{s.ljust(key_len_max)} {stats[s].rjust(val_len_max)}\n")

        if logger is not None:
            with open(out_filepath) as f:
                logger.info(f.read())

    @staticmethod
    def _times_between_to_histogram(preset_desc, output_dir, times_between_ms,
                                    out_filename="histogram"):
        bin_width_ms = 0.01

        fig = plt.figure()
        # Increase window size to improve visibility
        fig.set_size_inches(10, 5)

        plt.xlabel("Time between events [ms]")
        plt.ylabel("Number of occurrences")
        plt.title(preset_desc)
        plt.grid(True)

        plt.hist(times_between_ms, bins=np.arange(min(times_between_ms),
                                                  max(times_between_ms) + bin_width_ms,
                                                  bin_width_ms))

        plt.savefig(os.path.join(output_dir, f"{out_filename}.png"))

        # Save the same plot in log scale
        plt.yscale("log")
        plt.savefig(os.path.join(output_dir, f"{out_filename}_log.png"))

    @staticmethod
    def _times_between_to_interval_plot(preset_desc, output_dir, start_times, times_between_ms,
                                        out_filename="interval_plot"):
        fig = plt.figure()
        # Increase window size to improve visibility
        fig.set_size_inches(10, 5)

        plt.xlabel("Start event timestamp [s]")
        plt.ylabel("Time between events [ms]")
        plt.title(preset_desc)
        plt.grid(True)

        plt.plot(start_times, times_between_ms, ".")
        plt.savefig(os.path.join(output_dir, f"{out_filename}.png"))

    @staticmethod
    def _test_preset_parse_event(event_dict):
        if event_dict is None:
            return None, None

        name = event_dict.get("name")
        state = event_dict.get("state")

        if state is not None:
            state = EventState(state)
        else:
            # Assume "submit" event state by default
            state = EventState.SUBMIT

        return name, state

    def _test_preset_execute(self, test_preset, start_time, end_time):
        # Load preset description
        preset_name = test_preset.get("name")
        start_evt_name, start_evt_state = \
            StatsNordic._test_preset_parse_event(test_preset.get("start_event"))
        end_evt_name, end_evt_state = \
            StatsNordic._test_preset_parse_event(test_preset.get("end_event"))

        # Validate preset description
        if preset_name is None:
            raise ValueError("Invalid preset: No preset name")
        if start_evt_name is None:
            raise ValueError(f"Invalid preset: No start event name ({preset_name})")
        if end_evt_name is None:
            raise ValueError(f"Invalid preset: No end event name ({preset_name})")

        preset_desc = f"{preset_name}: {start_evt_name}({start_evt_state})->{end_evt_name}({end_evt_state})"
        self.logger.info(f"Execute test preset {preset_desc}")

        # Calculate event propagation times
        ts_start = self._get_timestamps(start_evt_name, start_evt_state, start_time, end_time)
        ts_end = self._get_timestamps(end_evt_name, end_evt_state, start_time, end_time)
        times_between_ms, ts_start, ts_end = self._calculate_times_between_ms(ts_start, ts_end)

        # Prepare output directory
        output_dir1 = f"{self.dataset_name}_from_{str(start_time)}_to_{str(end_time)}"
        output_dir1 = output_dir1.replace(".", "_")

        output_dir2 = preset_name.lower().replace(" ", "_").replace(".", "_")

        output_dir = os.path.join(OUTPUT_DIR_BASE, output_dir1, output_dir2)
        os.makedirs(output_dir, exist_ok=True)

        # Filter out outliers
        mask = StatsNordic._get_outlier_filter_mask(times_between_ms)
        times_between_ms_no_outliers = times_between_ms[mask]
        ts_start_no_outliers = ts_start[mask]

        # Store results in the output directory
        StatsNordic._times_between_to_stats_txt(preset_desc, output_dir, times_between_ms,
                                                logger=self.logger)
        StatsNordic._times_between_to_histogram(preset_desc + " no outliers", output_dir,
                                                times_between_ms_no_outliers,
                                                out_filename="histogram_no_outliers")
        StatsNordic._times_between_to_interval_plot(preset_desc, output_dir, ts_start,
                                                    times_between_ms)
        StatsNordic._times_between_to_interval_plot(preset_desc + " no outliers", output_dir,
                                                    ts_start_no_outliers,
                                                    times_between_ms_no_outliers,
                                                    out_filename="interval_plot_no_outliers")

        # Display all of the figures to the user
        plt.show()

    def calculate_stats(self, test_presets, start_time, end_time):
        for t in test_presets:
            try:
                self._test_preset_execute(t, start_time, end_time)
            except Exception as e:
                self.logger.warning(f"Exception: {e}")
