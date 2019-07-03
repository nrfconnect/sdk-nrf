import serial
import time
import re
import multiprocessing
import threading
import traceback

from user_logger import LogLevel
from zephyrlogparser_backend_generic import ZephyrLogParserGenericBackend


class ZephyrLogParserUartBackend(ZephyrLogParserGenericBackend):

    HELP_LINES = [
                    "port [arg]     - if [arg] provided sets UART serial device path to [arg]. "
                    "Otherwise print current serial device path.",
                    "baud [arg]     - if [arg] provided sets UART baudrate to [arg]. "
                    "Otherwise print current baudrate setting.",
                    "start          - initialize UART connection with specified settings",
                    "stop           - uninitialize UART connection with specified settings"
                 ]

    def __init__(self, module_id, stop_flag, elffile_path, serialport, baudrate=115200, cmd_queue=None,
                 stdout_queue=None, port=None, shellport=None):
        super().__init__(module_id, stop_flag, elffile_path, cmd_queue, stdout_queue, port, shellport, use_internal_timestamp=True)
        self.is_uart_enabled = False
        self.serialport = serialport
        self.seriallock = threading.Lock()
        self.baudrate = baudrate
        self.serdev = None
        self.log_message(LogLevel.INFO, "UART Backend Initialized")

    def _set_serial_port(self, serialport_path):
        self.serialport = serialport_path
        self.log_message(LogLevel.ERROR, "Changed serial port to: {}".format(self.serialport))

    def _set_baudrate(self, baudrate):
        self.baudrate = baudrate
        self.log_message(LogLevel.ERROR, "Baudrate changed to: {}".format(self.baudrate))

    def _parse_cmd(self, cmd):
        self.log_message(LogLevel.DEBUG, "got msg: {}".format(cmd))

        if cmd.startswith("start"):
            if not self.is_uart_enabled:
                try:
                    self.serdev = serial.Serial(self.serialport, self.baudrate, timeout=0)
                except IOError:
                    self.log_message(LogLevel.ERROR, "UART device on path: \"{}\" cannot be open".format(self.serialport))
                    return
                self.is_uart_enabled = True
                self.elfparser.wait_for_sync_frame()
                self.log_message(LogLevel.INFO, "Serial port device on path: \"{}\" successfully opened".format(self.serialport))
            else:
                self.log_message(LogLevel.INFO, "Serial port already opened")

        elif cmd.startswith("stop"):
            if self.is_uart_enabled:
                self.is_uart_enabled = False
                self.log_message(LogLevel.INFO, "Closing serial port: {}".format(self.serdev.name))
                with self.seriallock:
                    self.serdev.close()
            else:
                self.log_message(LogLevel.INFO, "Serial communication already stopped")

        elif cmd.startswith("port"):
            arg = self._get_command_arguments(cmd, arg_count=1)
            if arg:
                self._set_serial_port(arg[0])
            else:
                self.log_message(LogLevel.INFO, "Current serial port: {}".format(self.serialport))

        elif cmd.startswith("baud"):
            arg = self._get_command_arguments(cmd, arg_count=1)
            if arg:
                try:
                    self._set_baudrate(int(arg[0]))
                except ValueError:
                    self.log_message(LogLevel.INFO, "Wrong argument format")
            else:
                self.log_message(LogLevel.INFO, "Current baud rate setting: {}".format(self.baudrate))

        elif cmd.startswith("help"):
            [self.log_message(LogLevel.INFO, line) for line in self.HELP_LINES]

        else:
            self.log_message(LogLevel.ERROR, "Command not recognised")

    def _do_cyclic_task(self):
        if self.is_uart_enabled:
            with self.seriallock:
                bytes = 0
                try:
                    bytes = self.serdev.read(4096)
                except serial.SerialException:
                    self.log_message(LogLevel.ERROR, "UART read failed")
                    time.sleep(0.1)
                if bytes != 0:
                    bytes_parsed = [byte for byte in bytes]
                    output = self.elfparser.feed_with_bytes(bytes_parsed)
                    self._handle_parsed_input_from_backend_medium(output)

    def _gracefully_shutdown(self):
        if self.is_uart_enabled:
            self.serdev.close()