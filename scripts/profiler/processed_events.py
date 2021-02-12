#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from events import EventsData, TrackedEvent
import logging
import sys

class ProcessedEvents():
    def __init__(self):
        self.raw_data = EventsData([], {})
        self.tracked_events = []

        self.event_processing_start_id = None
        self.event_processing_end_id = None
        self.tracking_execution = True

        self.submit_event = None
        self.start_event = None

        self.logger = logging.getLogger('Processed Events')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(logging.WARNING)
        self.log_format = logging.Formatter(
            '[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)

    def match_event_processing(self):
        self.event_processing_start_id = self.raw_data.get_event_type_id(
            'event_processing_start')
        self.event_processing_end_id = self.raw_data.get_event_type_id(
            'event_processing_end')

        if (self.event_processing_start_id is None) or \
          (self.event_processing_end_id is None):
            self.tracking_execution = False
            for ev in self.raw_data.events:
                self.tracked_events.append(TrackedEvent(ev, None, None))
            return

        for i in range(1, len(self.raw_data.events)):
            if self.raw_data.events[i].type_id == self.event_processing_start_id:
                self.start_event = self.raw_data.events[i]
                for j in range(i - 1, -1, -1):
                    # comparing memory addresses of event processing start
                    # and event submit to identify matching events
                    if self.raw_data.events[j].data[0] == self.start_event.data[0]:
                        self.submit_event = self.raw_data.events[j]
                        break

            # comparing memory addresses of event processing start and end
            # to identify matching events
            if self.raw_data.events[i].type_id == self.event_processing_end_id:
                if self.submit_event is not None \
                        and self.raw_data.events[i].data[0] == self.start_event.data[0]:
                    self.tracked_events.append(
                        TrackedEvent(
                            self.submit_event,
                            self.start_event.timestamp,
                            self.raw_data.events[i].timestamp))
                    self.submit_event = None
