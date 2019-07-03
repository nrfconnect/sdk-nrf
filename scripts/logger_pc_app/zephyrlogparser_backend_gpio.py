import saleae
import multiprocessing
import threading
import time
import os
import csv
import warnings
import psutil
import socket
warnings.filterwarnings("ignore")

from zephyrlogparser_backend_generic import ZephyrLogParserGenericBackend
from user_logger import LogLevel

class ZephyrLogParserGpioBackend(ZephyrLogParserGenericBackend):

    HELP_LINES = [
                    "time [arg]     - if [arg] provided sets acquisition time to [arg]. "
                    "Otherwise print current acquisition time setting.",
                    "outfile [arg]  - if [arg] provided sets output file path to [arg]. "
                    "Otherwise print current acquisition time setting.",
                    "init           - initializes Saleae software",
                    "start          - starts acquisition with chosen settings",
                    "parse          - read file with data dumb and parse it into strings"
                 ]

    DEFAULT_HOST = 'localhost'
    DEFAULT_PORT = 10429

    SEVERITY_STRINGS = ["None", "Error", "Warning", "Info", "Debug"]

    def __init__(self, module_id, stop_flag, elffile_path, outputfile_path, capture_time, cmd_queue=None, stdout_queue=None, port=None):
        super().__init__(module_id, stop_flag, elffile_path, cmd_queue, stdout_queue, port,
                         shellport=None, use_internal_timestamp=False)
        self.output_filename = outputfile_path
        self.capture_time = capture_time
        self.is_driver_initialized = False
        self.log_message(LogLevel.INFO, "GPIO Backend Initialized")

    def _is_logic_software_running(self):
        for proc in psutil.process_iter():
            try:
                if proc.name().lower().startswith("logic"):
                    return True
            except psutil.NoSuchProcess:
                pass
        return False

    def _is_logic_software_connectable(self, host=DEFAULT_HOST, port=DEFAULT_PORT):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((host, port))
            sock.close()
            return True
        except ConnectionRefusedError:
            return False

    def _saleae_driver_init(self):
        self.logicanalyzer = saleae.Saleae()
        self.log_message(LogLevel.INFO, "Successfully connected to Saleae Software")
        self.logicanalyzer.set_capture_seconds(self.capture_time)

        #enforce proper sample rate
        rates = self.logicanalyzer.get_all_sample_rates()
        hi_rate = max(rates)
        self.logicanalyzer.set_sample_rate(hi_rate)
        self.log_message(LogLevel.INFO, "Sample rate set to {}".format(hi_rate[0]))
        if hi_rate[0] < 100000000:
            self.log_message(LogLevel.WARN, "Maximum sample rate is low. This may lead to data corruption.")

        self.is_driver_initialized = True
        pass

    def _launch_logic_software(self):
        if self._is_logic_software_running():
            if self._is_logic_software_connectable():
                self._saleae_driver_init()
            else:
                self.log_message(LogLevel.ERROR, "Saleae Software seems to be launched but cannot be connected to. "
                                                 "Did you enabled scripting server in Options? "
                                                 "Is Saleae window visible? Maybe it has not booted yet?")
        else:
            self.log_message(LogLevel.INFO, "Saleae Software is disabled. Trying to launch it now...")
            saleae.Saleae.launch_logic(10)
            self._saleae_driver_init()

    def _set_capture_time(self, timespan_in_seconds):
        self.capture_time = timespan_in_seconds
        if self.is_driver_initialized:
            self.logicanalyzer.set_capture_seconds(timespan_in_seconds)
        self.log_message(LogLevel.INFO, "Capture time set to: {}".format(timespan_in_seconds))

    def _set_output_file(self, filename):
        self.output_filename = filename + ".csv"
        self.log_message(LogLevel.INFO, "Output file set to: {}".format(self.output_filename))

    def _wait_for_data_export(self):
        while not self.logicanalyzer.is_processing_complete():
            time.sleep(1e-6)

    def _start_data_acquisition(self):
        self.log_message(LogLevel.INFO, "Started acquisition for {} seconds. Please wait...".format(self.capture_time))
        self.logicanalyzer.capture_start_and_wait_until_finished()
        self.log_message(LogLevel.INFO, "Acquisition finished. Exporting result to file {}. Please wait...".format(self.output_filename))

        script_path = os.getcwd()
        output_file_absolute_path = os.path.join(script_path, self.output_filename)
        self.logicanalyzer.export_data(output_file_absolute_path, csv_combined=False, csv_number_format='bin')
        self._wait_for_data_export()
        self.log_message(LogLevel.INFO, "Export finished. ")

    def _is_clock_rising(self, prev_clock, this_clock):
        if prev_clock == 0 and this_clock == 1:
            return True
        else:
            return False

    def _get_severity_level_string(self, severity_id):
        return self.SEVERITY_STRINGS[severity_id]

    def _get_data_clock(self, row):
        return int(row[9])

    def _get_timestamp(self, row):
        return row[0]

    def _get_data_byte(self, row):
        bits = row[1:9]
        bits_combined = sum([((int(bit)) << pos) for pos, bit in enumerate(bits)])
        return bits_combined

    def _is_hexdump_msg(self, value):
        if value == 0xFFFFFFFF:
            return True
        else:
            return False

    def _get_file_lines(self, filepath):
        return sum(1 for line in open(filepath))

    def _is_file_readable(self, filepath):
        try:
            fd = open(filepath, "r")
            fd.close()
            return True
        except IOError:
            return False

    def _parse_exported_data(self):
        self.log_message(LogLevel.INFO, "*** Parsing file: {} : ***".format(self.output_filename))

        byte_data = []
        time_data = []

        lines_amount = self._get_file_lines(self.output_filename)
        self.log_message(LogLevel.INFO, "File got {} lines".format(lines_amount))

        myfile = open(self.output_filename)
        reader = csv.reader(myfile, delimiter=',')
        next(reader, None)  # drop header
        first_row = next(reader, None)
        prev_clock = self._get_data_clock(first_row)

        was_clock_rising = False
        byte_to_append_previous = 0
        last_percent = 0
        for index, row in enumerate(reader):
            if self._is_module_enabled():
                this_clock = self._get_data_clock(row)
                if was_clock_rising:
                    if this_clock == 1:
                        new_byte = self._get_data_byte(row)
                        if new_byte > byte_to_append_previous:
                            byte_to_append_previous = new_byte
                    else:
                        byte_data.append(byte_to_append_previous)
                        was_clock_rising = False

                elif self._is_clock_rising(prev_clock, this_clock):
                    was_clock_rising = True
                    byte_to_append_previous = self._get_data_byte(row)
                    timestamp = self._get_timestamp(row)
                    time_data.append(timestamp.rstrip("0"))
                prev_clock = this_clock

                completness_percentage = int((index / lines_amount) * 100)
                if completness_percentage >= last_percent:
                    last_percent = completness_percentage + 5
                    self.log_message(LogLevel.INFO, "Collecting bytes: {}% of file".format(completness_percentage))
            else:
                return
        if was_clock_rising:
            byte_data.append(byte_to_append_previous)

        myfile.close()

        amount_of_bytes = len(byte_data)

        self.log_message(LogLevel.INFO, "Collected all {} bytes.".format(amount_of_bytes))

        get_timestamp = True
        first_byte_timestamp = ""
        strings_parsed = 0
        for idx, byte in enumerate(byte_data):
            if self._is_module_enabled():
                if get_timestamp:
                    # format timestamp to contain maximum 8 characters
                    first_byte_timestamp = (time_data[idx] + "00000000")[:8]
                    get_timestamp = False
                output = self.elfparser.feed_with_bytes([byte])
                if output:
                    strings_parsed += 1
                    line = output[0][1]
                    self.log_message(LogLevel.INFO, "[{}] {}".format(first_byte_timestamp, line))
                    get_timestamp = True
            else:
                return

        if lines_amount == 0:
            self.log_message(LogLevel.ERROR, "File empty - check if Saleae window contain any output. If not, check"
                                             "your cables. Otherwise Saleae could not save file in folder containing"
                                             "script. Check that folder permissions.")

        elif amount_of_bytes == 0:
            self.log_message(LogLevel.ERROR, "No bytes found in specified .csv file. Check if clock is properly"
                                             "supplied, because bytes are evaluated on clock rising edge.")

        elif strings_parsed == 0:
            self.log_message(LogLevel.ERROR, "No string parsed from received bytes. Check if sync frame is sent. "
                                             "Make sure if supplied .elf file is correct.")

    def _parse_cmd(self, cmd):
        self.log_message(LogLevel.DEBUG, "got msg {}".format(cmd))

        if cmd.startswith("time"):
            arg = self._get_command_arguments(cmd, arg_count=1)
            if arg:
                self._set_capture_time(float(arg[0]))
            else:
                self.log_message(LogLevel.INFO, "Current acquisition time is: {} seconds".format(self.capture_time))

        elif cmd.startswith("outfile"):
            arg = self._get_command_arguments(cmd, arg_count=1)
            if arg:
                self._set_output_file(arg[0])
            else:
                self.log_message(LogLevel.INFO, "Current output filename is: {}".format(self.output_filename))

        elif cmd.startswith("init"):
            self._launch_logic_software()

        elif cmd.startswith("parse"):
            self.elfparser.wait_for_sync_frame()
            if self._is_file_readable(self.output_filename):
                self._parse_exported_data()
            else:
                self.log_message(LogLevel.ERROR,
                                 "File containing logs to parse \"{}\" could not be opened".format(self.output_filename))

        elif cmd.startswith("start"):
            if self.is_driver_initialized:
                self._start_data_acquisition()
            else:
                self.log_message(LogLevel.INFO, "Initialize Saleae driver first. Type 'gpio init'")

        elif cmd.startswith("help"):
            [self.log_message(LogLevel.INFO, line) for line in self.HELP_LINES]

        else:
            self.log_message(LogLevel.ERROR, "Command not recognised")

    def _do_cyclic_task(self):
        time.sleep(1)

    def _gracefully_shutdown(self):
        pass