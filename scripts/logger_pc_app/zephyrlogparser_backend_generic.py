import multiprocessing
import threading
import user_logger
import time
import sys
import socket
import select
import queue

from user_logger import LogLevel
from user_logger import UserLogger
from queue_msg_bus import QueueMsgBus
from elffile_stringparser import ZephyrElfStringParser
from zephyrlogparser_shell import ZephyrLogParserShell

class ZephyrLogParserGenericBackend():
    def __init__(self, module_id, stop_flag, elffile_path, cmd_queue, stdout_queue, port, shellport, use_internal_timestamp):
        self.module_id = module_id
        self.cmd_bus = QueueMsgBus(cmd_queue, module_id.value)
        self.stop_flag = stop_flag

        self.elffile_path = elffile_path
        self.elfparser = ZephyrElfStringParser(self.elffile_path, use_internal_timestamp)

        self.worker_threads = []

        self.socket_writer_queue = None
        self.is_logger_socket_connected = threading.Event()
        if port:
            self.socket_writer_queue = queue.Queue()
            socket_writer = threading.Thread(target=self._manage_tcp_logger_loop, args=(port, ))
            self.worker_threads.append(socket_writer)
            socket_writer.start()

        self.userlogger = UserLogger(module_id, stdout_queue, self.socket_writer_queue, self.is_logger_socket_connected)

        if cmd_queue:
            buslistener = threading.Thread(target=self.listen_cmdbus_loop)
            self.worker_threads.append(buslistener)
            buslistener.start()

        self.is_shell_socket_connected = False
        if shellport:
            self.shell_output_queue = queue.Queue()
            shell_manager = threading.Thread(target=self._manage_tcp_shell_loop, args=(shellport, ))
            self.worker_threads.append(shell_manager)
            shell_manager.start()

    def log_message(self, log_level, log_string):
        self.userlogger.do_logging(log_level, log_string)

    def listen_cmdbus_loop(self):
        while self._is_module_enabled():
            try:
                cmd = self.cmd_bus.try_get()
                if cmd:
                    self._parse_cmd(cmd)
                time.sleep(1e-6)
            except KeyboardInterrupt:
                pass

    def run(self):
        while self._is_module_enabled():
            try:
                self._do_cyclic_task()
                time.sleep(1e-6)
            except KeyboardInterrupt:
                pass
        self._wait_for_threads_death()
        self._gracefully_shutdown()

    def _handle_parsed_input_from_backend_medium(self, input):
        for log_type, log_msg in input:
            if self.is_shell_socket_connected and log_type == self.elfparser.LOG_TYPE_SHELL:
                self.shell_output_queue.put(log_msg)
            else:
                self.log_message(LogLevel.INFO, log_msg)

    def _initialize_socket(self, port):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setblocking(False)
        server_socket.bind(('localhost', port))
        server_socket.listen()
        return server_socket

    def _manage_tcp_shell_loop(self, port):
        server_socket = self._initialize_socket(port)
        is_first_pass_of_listening = True

        while self._is_module_enabled():
            if not self.is_shell_socket_connected:
                if is_first_pass_of_listening:
                    self.log_message(LogLevel.INFO, "Shell waiting for TCP connection on port {}".format(port))
                    is_first_pass_of_listening = False
                try:
                    active_socket, _ = server_socket.accept()
                    self.is_shell_socket_connected = True
                    self.log_message(LogLevel.INFO, "Shell connected on port {}.".format(port))
                except BlockingIOError:
                    pass
            else:
                try:
                    try:
                        char = active_socket.recv(1)
                        if char:
                            self._send_over_backend_medium(char)
                        else:
                            raise ConnectionError
                    except BlockingIOError:
                        pass
                    try:
                        msg = self.shell_output_queue.get(block=False)
                        active_socket.send(msg)
                    except queue.Empty:
                        pass
                except ConnectionError:
                    self.is_shell_socket_connected = False
                    self.log_message(LogLevel.INFO, "Shell disconnected on port {}.".format(port))
                    is_first_pass_of_listening = True
            time.sleep(1e-6)

    def _manage_tcp_logger_loop(self, port):
        server_socket = self._initialize_socket(port)
        is_first_pass_of_listening = True

        while self._is_module_enabled():
            if not self.is_logger_socket_connected.is_set():
                if is_first_pass_of_listening:
                    self.log_message(LogLevel.INFO, "Waiting for TCP connection on port {}".format(port))
                    is_first_pass_of_listening = False
                try:
                    active_socket, _ = server_socket.accept()
                    output_func = lambda line_to_output: active_socket.send(line_to_output.encode())
                    cmd_parse_func = lambda input_line: self.cmd_bus.put_new_to_itself(input_line)
                    pretty_printer = ZephyrLogParserShell(output_func, cmd_parse_func, prompt=self.module_id.name + " ~$ ")
                    self.is_logger_socket_connected.set()
                    self.log_message(LogLevel.INFO, "TCP connected on port {}.".format(port))
                except BlockingIOError:
                    pass
            else:
                try:
                    try:
                        char = active_socket.recv(1)
                        if char:
                            pretty_printer.handle_input_char(char.decode())
                        else:
                            raise ConnectionError
                    except BlockingIOError:
                        pass
                    try:
                        msg = self.socket_writer_queue.get(block=False)
                        pretty_printer.handle_new_line(msg + "\r")
                    except BlockingIOError:
                        pass
                    except queue.Empty:
                        pass
                except ConnectionError:
                    self.is_logger_socket_connected.clear()
                    self.log_message(LogLevel.INFO, "TCP disconnected on port {}.".format(port))
                    is_first_pass_of_listening = True
            time.sleep(1e-6)

    def _get_command_arguments(self, cmd, arg_count=9999):
        cmd_split_to_string_and_args = cmd.split(" ")
        args = list(filter(None, cmd_split_to_string_and_args[1:]))
        if len(args) != arg_count and arg_count != 9999:
            return None
        else:
            return args

    def _is_module_enabled(self):
        if self.stop_flag.is_set():
            return False
        else:
            return True

    def _wait_for_threads_death(self):
        for thread in self.worker_threads:
            while thread.is_alive():
                pass

    def _send_over_backend_medium(self, char):
        raise NotImplementedError

    def _parse_cmd(self, cmd):
        raise NotImplementedError

    def _do_cyclic_task(self):
        raise NotImplementedError

    def _gracefully_shutdown(self):
        raise NotImplementedError
