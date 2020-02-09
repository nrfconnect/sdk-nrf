#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import matplotlib
from matplotlib.collections import PatchCollection
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button
from enum import Enum

import csv
import numpy as np
import sys
from time import sleep
import time
import threading
import logging

from events import Event, EventType, EventsData, TrackedEvent
from processed_events import ProcessedEvents
from plot_nordic_config import PlotNordicConfig


class MouseButton(Enum):
    LEFT = 1
    MIDDLE = 2
    RIGHT = 3

class DrawState():
    def __init__(self, timeline_width_init,
                 event_processing_rect_height, event_submit_markersize):
        self.timeline_max = 0
        self.timeline_width = timeline_width_init

        self.event_processing_rect_height = event_processing_rect_height
        self.event_submit_markersize = event_submit_markersize

        self.y_max = None
        self.y_height = None

        self.l_line = None
        self.l_line_coord = None
        self.r_line = None
        self.r_line_coord = None
        self.duration_marker = None
        self.pan_x_start = None

        self.paused = False

        self.ax = None

        self.selected_event_submit = None
        self.selected_event_processing = None

        self.selected_event_text = None
        self.selected_event_textbox = None

        self.added_time = 0
        self.synchronized_with_events = False
        self.stale_events_displayed = False

class PlotNordic():

    def __init__(self, log_lvl=logging.WARNING):
        plt.rcParams['toolbar'] = 'None'
        plt.ioff()
        self.plot_config = PlotNordicConfig
        self.draw_state = DrawState(
            self.plot_config['timeline_width_init'],
            self.plot_config['event_processing_rect_height'],
            self.plot_config['event_submit_markersize'])
        self.processed_events = ProcessedEvents()
        self.finish_event = None
        self.submitted_event_type = None

        self.temp_events = []

        self.logger = logging.getLogger('RTT Plot Nordic')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(log_lvl)
        self.log_format = logging.Formatter(
            '[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)


    def read_data_from_files(self, events_filename, events_types_filename):
        self.processed_events.raw_data.read_data_from_files(
            events_filename, events_types_filename)
        if not self.processed_events.raw_data.verify():
            self.logger.warning("Missing event descriptions")

    def write_data_to_files(self, events_filename, events_types_filename):
        self.processed_events.raw_data.write_data_to_files(
            events_filename, events_types_filename)

    def on_click_start_stop(self, event):
        if self.draw_state.paused:
            if self.draw_state.l_line is not None:
                self.draw_state.l_line.remove()
                self.draw_state.l_line = None
                self.draw_state.l_line_coord = None

            if self.draw_state.r_line is not None:
                self.draw_state.r_line.remove()
                self.draw_state.r_line = None
                self.draw_state.r_line_coord = None

            if self.draw_state.duration_marker is not None:
                self.draw_state.duration_marker.remove()

        self.draw_state.paused = not self.draw_state.paused

    def _prepare_plot(self, selected_events_types):

        self.draw_state.ax = plt.gca()
        self.draw_state.ax.set_navigate(False)

        fig = plt.gcf()
        fig.set_size_inches(
            self.plot_config['window_width_inch'],
            self.plot_config['window_height_inch'],
            forward=True)
        fig.canvas.draw()

        plt.xlabel("Time [s]")
        plt.title("Custom events")
        plt.grid(True)

        minimum = selected_events_types[0]
        maximum = selected_events_types[0]
        ticks = []
        labels = []
        for j in selected_events_types:
            if j != self.processed_events.event_processing_start_id \
              and j != self.processed_events.event_processing_end_id:
                if j > maximum:
                    maximum = j
                if j < minimum:
                    minimum = j
                ticks.append(j)
                labels.append(self.processed_events.raw_data.registered_events_types[j].name)
        plt.yticks(ticks, labels)

        # min and max range of y axis are bigger by one so markers fit nicely
        # on plot
        self.draw_state.y_max = maximum + 1
        self.draw_state.y_height = maximum - minimum + 2
        plt.ylim([minimum - 1, maximum + 1])

        self.draw_state.selected_event_textbox = self.draw_state.ax.text(
            0.05,
            0.95,
            self.draw_state.selected_event_text,
            fontsize=10,
            transform=self.draw_state.ax.transAxes,
            verticalalignment='top',
            bbox=dict(
                boxstyle='round',
                alpha=0.5,
                facecolor='linen'))
        self.draw_state.selected_event_textbox.set_visible(False)

        fig.canvas.mpl_connect('scroll_event', self.scroll_event)
        fig.canvas.mpl_connect('button_press_event', self.button_press_event)
        fig.canvas.mpl_connect('button_release_event',
                               self.button_release_event)
        fig.canvas.mpl_connect('resize_event', self.resize_event)
        fig.canvas.mpl_connect('close_event', self.close_event)

        plt.tight_layout()

        return fig

    def _get_relative_coords(self, event):
        # relative position of plot - x0, y0, width, height
        ax_loc = self.draw_state.ax.get_position().bounds
        window_size = plt.gcf().get_size_inches() * \
            plt.gcf().dpi  # window size - width, height
        x_rel = (event.x - ax_loc[0] * window_size[0]) \
                 / ax_loc[2] / window_size[0]
        y_rel = (event.y - ax_loc[1] * window_size[1]) \
                 / ax_loc[3] / window_size[1]
        return x_rel, y_rel

    def scroll_event(self, event):
        x_rel, y_rel = self._get_relative_coords(event)

        if event.button == 'up':
            if self.draw_state.paused:
                self.draw_state.timeline_max = self.draw_state.timeline_max - (1 - x_rel) * \
                    (self.draw_state.timeline_width - self.draw_state.timeline_width *
                     self.plot_config['timeline_scale_factor'])
            self.draw_state.timeline_width = self.draw_state.timeline_width * \
                self.plot_config['timeline_scale_factor']

        if event.button == 'down':
            if self.draw_state.paused:
                self.draw_state.timeline_max = self.draw_state.timeline_max + (1 - x_rel) * \
                    (self.draw_state.timeline_width / self.plot_config['timeline_scale_factor'] -
                     self.draw_state.timeline_width)
            self.draw_state.timeline_width = self.draw_state.timeline_width / \
                self.plot_config['timeline_scale_factor']

        self.draw_state.ax.set_xlim(
            self.draw_state.timeline_max -
            self.draw_state.timeline_width,
            self.draw_state.timeline_max)
        plt.draw()

    def _find_closest_event(self, x_coord, y_coord):
        if self.processed_events.tracking_execution:
            filtered_id = list(filter(lambda x: x.submit.type_id == round(y_coord),
                                      self.processed_events.tracked_events))
            if len(filtered_id) == 0:
                return None
            matching_processing = list(
                filter(
                    lambda x: x.proc_start_time < x_coord and x.proc_end_time > x_coord,
                    filtered_id))
            if len(matching_processing):
                return matching_processing[0]
            dists = list(map(lambda x: min([abs(x.submit.timestamp - x_coord),
               abs(x.proc_start_time - x_coord), abs(x.proc_end_time - x_coord)]), filtered_id))
            return filtered_id[np.argmin(dists)]
        else:
            filtered_id = list(filter(lambda x: x.type_id == round(y_coord), self.processed_events.raw_data.events))
            if len(filtered_id) == 0:
                return None
            dists = list(map(lambda x: abs(x.timestamp - x_coord), filtered_id))
            return filtered_id[np.argmin(dists)]

    def _stringify_time(time_seconds):
        if time_seconds > 0.1:
            return '%.5f' % (time_seconds) + ' s'

        return '%.5f' % (1000 * time_seconds) + ' ms'

    def button_press_event(self, event):
        x_rel, y_rel = self._get_relative_coords(event)

        if event.button == MouseButton.LEFT.value:
            self.draw_state.pan_x_start1 = x_rel

        if event.button == MouseButton.MIDDLE.value:
            if self.draw_state.selected_event_submit is not None:
                for i in self.draw_state.selected_event_submit:
                    i.remove()
                self.draw_state.selected_event_submit = None

            if self.draw_state.selected_event_processing is not None:
                self.draw_state.selected_event_processing.remove()
                self.draw_state.selected_event_processing = None

            self.draw_state.selected_event_textbox.set_visible(False)

            if x_rel > 1 or x_rel < 0 or y_rel > 1 or y_rel < 0:
                plt.draw()
                return

            coord_x = self.draw_state.timeline_max - \
                (1 - x_rel) * self.draw_state.timeline_width
            coord_y = self.draw_state.y_max - \
                (1 - y_rel) * self.draw_state.y_height
            selected_event = self._find_closest_event(coord_x, coord_y)
            if selected_event is None:
                return

            if self.processed_events.tracking_execution:
                event_submit = selected_event.submit
            else:
                event_submit = selected_event

            self.draw_state.selected_event_submit = self.draw_state.ax.plot(
                event_submit.timestamp,
                event_submit.type_id,
                markersize=2*self.draw_state.event_submit_markersize,
                color='g',
                marker='o',
                linestyle=' ')

            if self.processed_events.tracking_execution:
                self.draw_state.selected_event_processing = matplotlib.patches.Rectangle(
                    (selected_event.proc_start_time,
                    selected_event.submit.type_id -
                    self.draw_state.event_processing_rect_height),
                    selected_event.proc_end_time -
                    selected_event.proc_start_time,
                    2*self.draw_state.event_processing_rect_height,
                    color='g')
                self.draw_state.ax.add_artist(
                    self.draw_state.selected_event_processing)

            self.draw_state.selected_event_text = \
                self.processed_events.raw_data.registered_events_types[event_submit.type_id].name + '\n'
            self.draw_state.selected_event_text += 'Submit: ' + \
                PlotNordic._stringify_time(event_submit.timestamp) + '\n'
            if self.processed_events.tracking_execution:
                self.draw_state.selected_event_text += 'Processing start: ' + \
                    PlotNordic._stringify_time(
                        selected_event.proc_start_time) + '\n'
                self.draw_state.selected_event_text += 'Processing end: ' + \
                    PlotNordic._stringify_time(
                        selected_event.proc_end_time) + '\n'
                self.draw_state.selected_event_text += 'Processing time: ' + \
                    PlotNordic._stringify_time(selected_event.proc_end_time - \
                        selected_event.proc_start_time) + '\n'

            ev_type = self.processed_events.raw_data.registered_events_types[event_submit.type_id]

            for i in range(0, len(ev_type.data_descriptions)):
                if ev_type.data_descriptions[i] == 'mem_address':
                    continue
                self.draw_state.selected_event_text += ev_type.data_descriptions[i] + ' = '
                self.draw_state.selected_event_text += str(event_submit.data[i]) + '\n'

            self.draw_state.selected_event_textbox.set_visible(True)
            self.draw_state.selected_event_textbox.set_text(
                self.draw_state.selected_event_text)

            plt.draw()

        if event.button == MouseButton.RIGHT.value:
            self.draw_state.pan_x_start2 = x_rel

    def button_release_event(self, event):
        x_rel, y_rel = self._get_relative_coords(event)

        if event.button == MouseButton.LEFT.value:
            if self.draw_state.paused:
                if abs(x_rel - self.draw_state.pan_x_start1) < 0.01:
                    if self.draw_state.l_line is not None:
                        self.draw_state.l_line.remove()
                        self.draw_state.l_line = None
                        self.draw_state.l_line_coord = None

                    if x_rel >= 0 and x_rel <= 1 and y_rel >= 0 and y_rel <= 1:
                        self.draw_state.l_line_coord = self.draw_state.timeline_max - \
                            (1 - x_rel) * self.draw_state.timeline_width
                        self.draw_state.l_line = plt.axvline(
                            self.draw_state.l_line_coord)
                    plt.draw()

                else:
                    self.draw_state.timeline_max = self.draw_state.timeline_max - \
                        (x_rel - self.draw_state.pan_x_start1) * \
                        self.draw_state.timeline_width
                    self.draw_state.ax.set_xlim(
                        self.draw_state.timeline_max -
                        self.draw_state.timeline_width,
                        self.draw_state.timeline_max)
                    plt.draw()

        if event.button == MouseButton.RIGHT.value:
            if self.draw_state.paused:
                if abs(x_rel - self.draw_state.pan_x_start2) < 0.01:
                    if self.draw_state.r_line is not None:
                        self.draw_state.r_line.remove()
                        self.draw_state.r_line = None
                        self.draw_state.r_line_coord = None

                    if x_rel >= 0 and x_rel <= 1 and y_rel >= 0 and y_rel <= 1:
                        self.draw_state.r_line_coord = self.draw_state.timeline_max - \
                            (1 - x_rel) * self.draw_state.timeline_width
                        self.draw_state.r_line = plt.axvline(
                            self.draw_state.r_line_coord, color='r')
                    plt.draw()

        if self.draw_state.r_line_coord is not None and self.draw_state.l_line_coord is not None:
            if self.draw_state.duration_marker is not None:
                self.draw_state.duration_marker.remove()
            bigger_coord = max(
                self.draw_state.r_line_coord,
                self.draw_state.l_line_coord)
            smaller_coord = min(
                self.draw_state.r_line_coord,
                self.draw_state.l_line_coord)
            self.draw_state.duration_marker = plt.annotate(
                s=PlotNordic._stringify_time(
                    bigger_coord - smaller_coord), xy=(
                    smaller_coord, 0.5), xytext=(
                    bigger_coord, 0.5), arrowprops=dict(
                    arrowstyle='<->'))
        else:
            if self.draw_state.duration_marker is not None:
                self.draw_state.duration_marker.remove()
                self.draw_state.duration_marker = None

    def resize_event(self, event):
        plt.tight_layout()

    def close_event(self, event):
        if self.finish_event is not None:
            self.finish_event.set()
        plt.close('all')
        sys.exit()

    def animate_events_real_time(self, fig, selected_events_types, one_line):
        rects = []
        finished = False
        events = []
        xranges = []
        for i in range(0, len(selected_events_types)):
            xranges.append([])
        yranges = []
        while not self.queue.empty():
            event = self.queue.get()
            if event is None:
                self.logger.info("Stopped collecting new events")
                self.close_event(None)

            if self.processed_events.tracking_execution:
                if event.type_id == self.processed_events.event_processing_start_id:
                    self.processed_events.start_event = event
                    for i in range(
                            len(self.temp_events) - 1, -1, -1):
                        # comparing memory addresses of event processing start
                        # and event submit to identify matching events
                        if self.temp_events[i].data[0] == self.processed_events.start_event.data[0]:
                            self.processed_events.submit_event = self.temp_events[i]
                            events.append(self.temp_events[i])
                            self.submitted_event_type = self.processed_events.submit_event.type_id
                            del(self.temp_events[i])
                            break

                elif event.type_id == self.processed_events.event_processing_end_id:
                    # comparing memory addresses of event processing start and
                    # end to identify matching events
                    if self.submitted_event_type is not None and event.data[0] \
                             == self.processed_events.start_event.data[0]:
                        rects.append(
                            matplotlib.patches.Rectangle(
                                (self.processed_events.start_event.timestamp,
                                 self.processed_events.submit_event.type_id -
                                 self.draw_state.event_processing_rect_height/2),
                                event.timestamp -
                                self.processed_events.start_event.timestamp,
                                self.draw_state.event_processing_rect_height,
                                edgecolor='black'))
                        self.processed_events.tracked_events.append(
                            TrackedEvent(
                                self.processed_events.submit_event,
                                self.processed_events.start_event.timestamp,
                                event.timestamp))
                        self.submitted_event_type = None
                else:
                    self.temp_events.append(event)
                    if event.timestamp > time.time() - self.start_time + self.draw_state.added_time - \
                            0.2 * self.draw_state.timeline_width:
                        self.draw_state.added_time += 0.05

                    if event.timestamp < time.time() - self.start_time + self.draw_state.added_time - \
                            0.8 * self.draw_state.timeline_width:
                        self.draw_state.added_time -= 0.05

                    events.append(event)
            else:
                events.append(event)
                self.processed_events.raw_data.events.append(event)

        # translating plot
        if not self.draw_state.synchronized_with_events:
            # ignore translating plot for stale events
            if not self.draw_state.stale_events_displayed:
                self.draw_state.stale_events_displayed = True
            else:
            # translate plot for new events
                if len(events) != 0:
                    self.draw_state.added_time = events[-1].timestamp - \
                                                   0.3 * self.draw_state.timeline_width
                    self.draw_state.synchronized_with_events = True

        if not self.draw_state.paused:
            self.draw_state.timeline_max = time.time() - self.start_time + \
                self.draw_state.added_time
            self.draw_state.ax.set_xlim(
                self.draw_state.timeline_max -
                self.draw_state.timeline_width,
                self.draw_state.timeline_max)

        # plotting events
        y = list(map(lambda x: x.type_id, events))
        x = list(map(lambda x: x.timestamp, events))
        self.draw_state.ax.plot(
            x,
            y,
            marker='o',
            linestyle=' ',
            color='r',
            markersize=self.draw_state.event_submit_markersize)

        self.draw_state.ax.add_collection(PatchCollection(rects))
        plt.gcf().canvas.flush_events()

    def plot_events_real_time(
            self,
            queue,
            finish_event,
            selected_events_types=None,
            one_line=False):
        self.start_time = time.time()
        self.queue = queue

        self.finish_event = finish_event
        self.processed_events.raw_data.registered_events_types = queue.get()

        self.processed_events.match_event_processing()
        if selected_events_types is None:
            selected_events_types = list(
                self.processed_events.raw_data.registered_events_types.keys())

        self.processed_events.event_processing_start_id = \
            self.processed_events.raw_data.get_event_type_id('event_processing_start')
        self.processed_events.event_processing_end_id = \
            self.processed_events.raw_data.get_event_type_id('event_processing_end')

        if (self.processed_events.event_processing_start_id is None) or (
                self.processed_events.event_processing_end_id is None):
            self.processed_events.tracking_execution = False
        fig = self._prepare_plot(selected_events_types)

        self.start_stop_ax = plt.axes([0.8, 0.025, 0.1, 0.04])
        self.start_stop_button = Button(self.start_stop_ax, 'Start/Stop')
        self.start_stop_button.on_clicked(self.on_click_start_stop)
        plt.sca(self.draw_state.ax)

        ani = animation.FuncAnimation(
            fig,
            self.animate_events_real_time,
            fargs=[
                selected_events_types,
                one_line],
            interval=self.plot_config['refresh_time'])
        plt.show()

    def plot_events_from_file(
            self, selected_events_types=None, one_line=False):
        self.draw_state.paused = True
        if len(self.processed_events.raw_data.events) == 0 or \
                len(self.processed_events.raw_data.registered_events_types) == 0:
            self.logger.error("Please read some events data before plotting")

        if selected_events_types is None:
            selected_events_types = list(
                self.processed_events.raw_data.registered_events_types.keys())

        self.processed_events.match_event_processing()
        fig = self._prepare_plot(selected_events_types)

        x = list(map(lambda x: x.submit.timestamp, self.processed_events.tracked_events))
        y = list(map(lambda x: x.submit.type_id, self.processed_events.tracked_events))
        self.draw_state.ax.plot(
            x,
            y,
            marker='o',
            linestyle=' ',
            color='r',
            markersize=self.draw_state.event_submit_markersize)

        if self.processed_events.tracking_execution:
            rects = []
            for ev in self.processed_events.tracked_events:
                rects.append(
                    matplotlib.patches.Rectangle(
                        (ev.proc_start_time,
                         ev.submit.type_id - self.draw_state.event_processing_rect_height/2),
                         ev.proc_end_time - ev.proc_start_time,
                         self.draw_state.event_processing_rect_height,
                         edgecolor='black'))

            self.draw_state.ax.add_collection(PatchCollection(rects))

        self.draw_state.timeline_max = max(x) + 1
        self.draw_state.timeline_width = max(x) - min(x) + 2
        self.draw_state.ax.set_xlim([min(x) - 1, max(x) + 1])

        plt.draw()
        plt.show()
