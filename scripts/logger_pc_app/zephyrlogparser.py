import argparse
import json
import multiprocessing
import sys
import time
import threading
import subprocess
import queue
from enum import Enum
import os

from getch import getChar
from user_logger import UserLogger
from queue_msg_bus import QueueMsgBus
from zephyrlogparser_config import BackendsIDs
from zephyrlogparser_backend_uart import ZephyrLogParserUartBackend
from zephyrlogparser_backend_gpio import ZephyrLogParserGpioBackend
from zephyrlogparser_backend_rtt import ZephyrLogParserRttBackend
from zephyrlogparser_shell import ZephyrLogParserShell

HELP_LINE = "quit           - stops application\n" \
            "[backend] help - help regarding specified backend"

cmd_queue = multiprocessing.Queue()
cmd_bus = QueueMsgBus(cmd_queue)

stdout_queue = multiprocessing.Queue()

def get_current_time_ms():
    return int(round(time.time() * 1000))

def background_job_loop(*args):
    backend = args[0]
    backend_obj = backend(*args[1:])
    backend_obj.run()

def wait_for_process_to_finish(process):
    TIMEOUT = 2000
    starttime = get_current_time_ms()
    while get_current_time_ms() - starttime < TIMEOUT:
        if process.is_alive():
            time.sleep(0.01)
        else:
            return
    process.terminate()
    starttime = get_current_time_ms()
    while get_current_time_ms() - starttime < TIMEOUT:
        if process.is_alive():
            time.sleep(0.01)
        else:
            return
    print("Process with PID {} could not be stopped.".format(process.pid))

def pretty_console_printer_loop(stop_flag, stdout_queue, pretty_printer):
    global current_user_input
    while not stop_flag.is_set():
        try:
            try:
                line_to_print_on_stdout = stdout_queue.get(block=False)
                pretty_printer.handle_new_line(line_to_print_on_stdout)
            except queue.Empty:
                pass
            time.sleep(1e-6)
        except KeyboardInterrupt:
            pass

def parse_user_input(input_line):
    if input_line.startswith("quit"):
        raise KeyboardInterrupt
    elif input_line.startswith("help"):
        stdout_queue.put(HELP_LINE)
    else:
        for backend in BackendsIDs:
            if input_line.startswith(backend.name.lower() + ' '):
                backend_id = backend.value
                cmd_val = input_line[len(backend.name + ' '):]
                cmd_bus.put_new(backend_id, cmd_val)
                break

def stdout_write(string):
    sys.stdout.write(string)
    sys.stdout.flush()

if __name__ == '__main__':

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    with open("logparser_config.json", "r") as read_file:
        config = json.load(read_file)
    for backend in config:
        backend_group = parser.add_argument_group(backend)
        for args in config[backend]:
                try:
                    choices = args["Choices"]
                except KeyError:
                    choices = None
                try:
                    required = args["Required"]
                except KeyError:
                    required = False
                if args["Name"].lower().startswith("enable"):
                    backend_group.add_argument(args["AsArg"], help=args["Help"],
                                               default=args["Default"],
                                               required=required,
                                               action='store_true')
                else:
                    backend_group.add_argument(args["AsArg"], help=args["Help"],
                                               default=args["Default"],
                                               type=type(args["Default"]),
                                               choices=choices,
                                               required=required,
                                               action=None)





    args = parser.parse_args()
    arg_dict = vars(args)

    shell_port_dict = {"uart": None, "rtt": None}

    elf_file_path = args.elf

    global_stop_flag = multiprocessing.Event()

    process_pool = []

    pretty_printer = ZephyrLogParserShell(stdout_write, parse_user_input)
    pretty_printer_proc = threading.Thread(target = pretty_console_printer_loop, args = (global_stop_flag, stdout_queue, pretty_printer))
    process_pool.append(pretty_printer_proc)

    if arg_dict["gpio_on"]:
        outfile = arg_dict["outfile"]
        capture_time = arg_dict["capture_time"]
        log_tcp_port = arg_dict["tcp_gpio"]
        backend_gpio_proc = multiprocessing.Process(target = background_job_loop,
                                       args = (ZephyrLogParserGpioBackend, BackendsIDs.GPIO, global_stop_flag,
                                               elf_file_path, outfile, capture_time, cmd_queue, stdout_queue, log_tcp_port
                                               ))
        process_pool.append(backend_gpio_proc)

    if arg_dict["uart_on"]:
        devport = arg_dict["serial"]
        baud = arg_dict["baud"]
        log_tcp_port = arg_dict["tcp_uart"]
        shell_tcp_port = shell_port_dict["uart"]
        backend_uart_proc = multiprocessing.Process(target = background_job_loop,
                                       args = (ZephyrLogParserUartBackend, BackendsIDs.UART, global_stop_flag,
                                               elf_file_path, devport, baud, cmd_queue, stdout_queue, log_tcp_port, shell_tcp_port
                                               ))
        process_pool.append(backend_uart_proc)

    if arg_dict["rtt_on"]:
        log_tcp_port = arg_dict["tcp_rtt"]
        shell_tcp_port = shell_port_dict["rtt"]
        backend_rtt_proc = multiprocessing.Process(target=background_job_loop,
                                                    args=(ZephyrLogParserRttBackend, BackendsIDs.RTT, global_stop_flag,
                                                          elf_file_path, cmd_queue, stdout_queue, log_tcp_port, shell_tcp_port
                                                          ))
        process_pool.append(backend_rtt_proc)

    if len(process_pool) == 1:
        print("At least one backend has to be initialized.")
        exit()

    for process in process_pool:
        process.start()

    try:
        while True:
            char = getChar()
            pretty_printer.handle_input_char(char)
    except KeyboardInterrupt:
        print("Interrupted by user.")
    finally:
        print("Cleaning up...")
        global_stop_flag.set()
        for process in process_pool:
            wait_for_process_to_finish(process)