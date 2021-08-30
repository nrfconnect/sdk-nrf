#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from processed_events import ProcessedEvents
from enum import Enum
import matplotlib.pyplot as plt
import numpy as np
import logging
import os


OUTPUT_FOLDER = "data_stats/"


class EventState(Enum):
    SUBMIT = 1
    PROC_START = 2
    PROC_END = 3


class StatsNordic():
    def __init__(self, events_filename, events_types_filename, log_lvl):
        self.data_name = events_filename.split('.')[0]
        self.processed_data = ProcessedEvents()
        self.processed_data.read_data_from_files(events_filename, events_types_filename)

        self.logger = logging.getLogger('Stats Nordic')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(log_lvl)
        self.log_format = logging.Formatter(
            '[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)

    def calculate_stats_preset1(self, start_meas, end_meas):
        self.time_between_events("hid_mouse_event_dongle", EventState.SUBMIT,
                                 "hid_report_sent_event_device", EventState.SUBMIT,
                                 0.05, start_meas, end_meas)
        self.time_between_events("hid_mouse_event_dongle", EventState.SUBMIT,
                                 "hid_report_sent_event_device", EventState.SUBMIT,
                                 0.05, start_meas, end_meas)
        self.time_between_events("hid_report_sent_event_dongle", EventState.SUBMIT,
                                 "hid_report_sent_event_dongle", EventState.SUBMIT,
                                 0.05, start_meas, end_meas)
        self.time_between_events("hid_mouse_event_dongle", EventState.SUBMIT,
                                 "hid_report_sent_event_dongle", EventState.SUBMIT,
                                 0.05, start_meas, end_meas)
        self.time_between_events("hid_mouse_event_device", EventState.SUBMIT,
                                 "hid_mouse_event_dongle", EventState.SUBMIT,
                                 0.05, start_meas, end_meas)
        plt.show()

    def _get_timestamps(self, event_name, event_state, start_meas, end_meas):
        event_type_id = self.processed_data.get_event_type_id(event_name)
        if event_type_id is None:
            self.logger.error("Event name not found: " + event_name)
            return None
        if not self.processed_data.is_event_tracked(event_type_id) and event_state != EventState.SUBMIT:
            self.logger.error("This event is not tracked: " + event_name)
            return None

        trackings = list(filter(lambda x:
                      x.submit.type_id == event_type_id,
                      self.processed_data.tracked_events))

        if not isinstance(event_state, EventState):
            self.logger.error("Event state should be EventState enum")
            return None

        if event_state == EventState.SUBMIT:
            timestamps = np.fromiter(map(lambda x: x.submit.timestamp, trackings),
                                     dtype=np.float)
        elif event_state == EventState.PROC_START:
            timestamps = np.fromiter(map(lambda x: x.proc_start_time, trackings),
                                     dtype=np.float)
        elif event_state == EventState.PROC_END:
            timestamps = np.fromiter(map(lambda x: x.proc_end_time, trackings),
                                     dtype=np.float)

        timestamps = timestamps[np.where((timestamps > start_meas)
                                         & (timestamps < end_meas))]

        return timestamps

    @staticmethod
    def calculate_times_between(start_times, end_times):
        if end_times[0] <= start_times[0]:
            end_times = end_times[1:]
        if len(start_times) > len(end_times):
            start_times = start_times[:-1]

        return (end_times - start_times) * 1000

    @staticmethod
    def prepare_stats_txt(times_between):
        stats_text = "Max time: "
        stats_text += "{0:.3f}".format(max(times_between)) + "ms\n"
        stats_text += "Min time: "
        stats_text += "{0:.3f}".format(min(times_between)) + "ms\n"
        stats_text += "Mean time: "
        stats_text += "{0:.3f}".format(np.mean(times_between)) + "ms\n"
        stats_text += "Std dev of time: "
        stats_text += "{0:.3f}".format(np.std(times_between)) + "ms\n"
        stats_text += "Median time: "
        stats_text += "{0:.3f}".format(np.median(times_between)) + "ms\n"
        stats_text += "Number of records: {}".format(len(times_between)) + "\n"

        return stats_text

    def time_between_events(self, start_event_name, start_event_state,
                            end_event_name, end_event_state, hist_bin_width=0.01,
                            start_meas=0, end_meas=float('inf')):
        self.logger.info("Stats calculating: {}->{}".format(start_event_name,
                                                            end_event_name))

        start_times = self._get_timestamps(start_event_name, start_event_state,
                                           start_meas, end_meas)
        end_times = self._get_timestamps(end_event_name, end_event_state,
                                           start_meas, end_meas)

        if start_times is None or end_times is None:
            return

        if len(start_times) == 0:
            self.logger.error("No events logged: " + start_event_name)
            return

        if len(end_times) == 0:
            self.logger.error("No events logged: " + end_event_name)
            return

        if len(start_times) != len(end_times):
            self.logger.error("Number of start_times and end_times is not equal")
            self.logger.error("Got {} start_times and {} end_times".format(
                len(start_times), len(end_times)))

            return

        times_between = self.calculate_times_between(start_times, end_times)
        stats_text = self.prepare_stats_txt(times_between)

        plt.figure()

        ax = plt.gca()
        ax.text(0.05,
                0.95,
                stats_text,
                transform=ax.transAxes,
                fontsize=12,
                verticalalignment='top',
                bbox=dict(boxstyle='round',
                            alpha=0.5,
                            facecolor='linen'))

        plt.xlabel('Duration[ms]')
        plt.ylabel('Number of occurrences')

        event_status_str = {
                        EventState.SUBMIT : "submission",
                        EventState.PROC_START : "processing start",
                        EventState.PROC_END : "processing end"
        }

        title = "From " + start_event_name + ' ' + \
                event_status_str[start_event_state] + "\nto " + \
                end_event_name + ' ' + event_status_str[end_event_state] + \
                ' (' + self.data_name + ')'
        plt.title(title)
        plt.hist(times_between, bins = (int)((max(times_between) - min(times_between))
                                        / hist_bin_width))

        plt.yscale('log')
        plt.grid(True)

        if end_meas == float('inf'):
            end_meas_string = 'inf'
        else:
            end_meas_string = int(end_meas)
        dir_name = "{}{}_{}_{}/".format(OUTPUT_FOLDER, self.data_name,
                                        int(start_meas), end_meas_string)
        if not os.path.exists(dir_name):
            os.makedirs(dir_name)

        plt.savefig(dir_name +
                    title.lower().replace(' ', '_').replace('\n', '_') +'.png')
