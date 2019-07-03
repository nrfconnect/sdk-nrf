from enum import Enum
import socket
import time
import threading
import queue

class LogLevel(Enum):
    ERROR = 0
    WARN = 1
    INFO = 2
    DEBUG = 3

class UserLogger:
    def __init__(self, module_id, stdout_queue, socket_writer_queue, socket_status_flag):
        self.module_id = module_id
        self.stdout_queue = stdout_queue
        self.socket_writer_queue = socket_writer_queue
        self.socket_status_flag = socket_status_flag
        self.log_levels_switches = [True for i in range(len(LogLevel))]

    def build_log_line(self, log_lvl, log_msg):
        prefix = "[{}]".format(self._get_log_level_string(log_lvl))
        source = "<{}>".format(self._get_log_source_string(self.module_id))
        line_to_log = "{0:7} {1:7} {2}".format(prefix, source, log_msg.strip())
        return line_to_log

    def _log_output(self, msg):
        if self.stdout_queue:
            self.stdout_queue.put(msg)
        else:
            print(msg)
        if self.socket_status_flag.is_set():
            self.socket_writer_queue.put(msg)

    def _get_log_source_string(self, log_source_to_stringify):
        return log_source_to_stringify.name

    def _get_log_level_string(self, log_lvl_to_stringify):
        return log_lvl_to_stringify.name

    def do_logging(self, log_lvl, log_msg):
        if self._is_log_level_enabled(log_lvl):
            self._log_output(self.build_log_line(log_lvl, log_msg))

    def _is_log_level_enabled(self, log_lvl_to_check):
        return self.log_levels_switches[log_lvl_to_check.value]



