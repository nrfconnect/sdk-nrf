# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

from events import EventsData
from processed_events import ProcessedEvents
from enum import Enum
import matplotlib.pyplot as plt
import numpy as np
import logging
import csv

class EventState(Enum):
    SUBMIT = 1
    PROC_START = 2
    PROC_END = 3

class StatsNordic():
    def __init__(self, events_filename, events_types_filename, log_lvl):
        self.data_name = events_filename.split('.')[0]
        self.processed_data = ProcessedEvents()
        self.processed_data.raw_data.read_data_from_files(
                                      events_filename, events_types_filename)
        self.processed_data.match_event_processing()
        self.logger = logging.getLogger('Stats Nordic')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(log_lvl)
        self.log_format = logging.Formatter(
            '[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)

    def calculate_stats_preset1(self):
        self.time_between_events("hid_report_sent_event", EventState.PROC_END,
                                 "motion_event", EventState.SUBMIT,
                                 4000)
        self.time_between_events("motion_event", EventState.SUBMIT,
                                 "hid_report_sent_event", EventState.PROC_END,
                                 4000)
        plt.show()

    def _get_timestamps(self, event_name, event_state):
        event_type_id = self.processed_data.raw_data.get_event_type_id(event_name)
        if event_type_id == None:
            self.logger.error("Event name not found: " + event_name)
            return None

        trackings = list(filter(lambda x:
                      x.submit.type_id == event_type_id,
                      self.processed_data.tracked_events))

        if type(event_state) is not EventState:
            self.logger.error("Event state should be EventState enum")
            return None

        if event_state == EventState.SUBMIT:
            timestamps = list(map(lambda x: x.submit.timestamp, trackings))
        if event_state == EventState.PROC_START:
            timestamps = list(map(lambda x: x.proc_start_time, trackings))
        if event_state == EventState.PROC_END:
            timestamps = list(map(lambda x: x.proc_end_time, trackings))

        return timestamps

    def time_between_events(self, start_event_name, start_event_state,
                            end_event_name, end_event_state, hist_bin_cnt):

        if not self.processed_data.tracking_execution:
            if start_event_state != EventState.SUBMIT or \
              end_event_state != EventState.SUBMIT:
                self.logger.error("Events processing is not tracked: " + \
                                  start_event_name + "->" + end_event_name)
                return

        start_times = self._get_timestamps(start_event_name, start_event_state)
        end_times = self._get_timestamps(end_event_name, end_event_state)

        if start_times is None or end_times is None:
            return

        times_between = []
        for start in start_times:
            for end in end_times:
                if start < end:
                    #converting times to ms - multiply by 1000
                    times_between.append((end - start) * 1000)
                    break

        plt.figure()
        stats_text = "Max time: "
        stats_text += "{0:.3f}".format(max(times_between)) + "ms\n"
        stats_text += "Min time: "
        stats_text += "{0:.3f}".format(min(times_between)) + "ms\n"
        stats_text += "Mean time: "
        stats_text += "{0:.3f}".format(np.mean(times_between)) + "ms\n"

        ax = plt.gca()
        stats_textbox = ax.text(0.05,
                                0.95,
                                stats_text,
                                transform=ax.transAxes,
                                fontsize=12,
                                verticalalignment='top',
                                bbox=dict(boxstyle='round',
                                          alpha=0.5,
                                          facecolor='linen'))

        plt.xlabel('Duration[ms]')
        plt.ylabel('Number of occurences')

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

        plt.hist(times_between, bins = hist_bin_cnt)
        plt.yscale('log')
        plt.grid(True)
