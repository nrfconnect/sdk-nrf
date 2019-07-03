import threading
import time
from pynrfjprog import API

from user_logger import LogLevel
from zephyrlogparser_backend_generic import ZephyrLogParserGenericBackend


class ZephyrLogParserRttBackend(ZephyrLogParserGenericBackend):

    HELP_LINES = [
                    "start          - initialize RTT connection",
                    "stop           - uninitialize RTT connection"
                 ]

    def __init__(self, module_id, stop_flag, elffile_path, cmd_queue=None, stdout_queue=None, port=None, shellport=None):
        super().__init__(module_id, stop_flag, elffile_path, cmd_queue, stdout_queue, port, shellport, use_internal_timestamp=True)
        self.is_rtt_enabled = False
        self.rtt_lock = threading.Lock()
        self.device_name = "NRF52"
        self.log_message(LogLevel.INFO, "RTT Backend Initialized")
        self.restart_timer = 0

    def _set_rtt_connection(self):
        self.jlink = API.API(self.device_name)

        self.jlink.open()
        self.jlink.enum_emu_snr()
        self.jlink.connect_to_emu_without_snr()
        self.jlink.rtt_start()

        get_current_time_ms = lambda: int(round(time.time() * 1000))
        TIMEOUT = 2000
        starttime = get_current_time_ms()
        while get_current_time_ms() - starttime < TIMEOUT:
            try:
                self.jlink.rtt_read(0, 0, encoding=None)
                return True
            except API.APIError:
                pass
            time.sleep(0.01)
        return False

    def _parse_cmd(self, cmd):
        self.log_message(LogLevel.DEBUG, "got msg: {}".format(cmd))

        if cmd.startswith("start"):
            if not self.is_rtt_enabled:
                if self._set_rtt_connection():
                    self.is_rtt_enabled = True
                    self.elfparser.wait_for_sync_frame()
                    self.log_message(LogLevel.INFO, "RTT device \"{}\" successfully connected to.".format(self.device_name))
                else:
                    self.log_message(LogLevel.ERROR, "RTT connection failed.")
            else:
                self.log_message(LogLevel.ERROR, "RTT device connection already started.")

        elif cmd.startswith("stop"):
            if self.is_rtt_enabled:
                self.is_rtt_enabled = False
                self.log_message(LogLevel.INFO, "RTT connection to \"{}\" closed.".format(self.device_name))
                with self.rtt_lock:
                    self.jlink.close()
            else:
                self.log_message(LogLevel.ERROR, "RTT device connection already stopped.")

        elif cmd.startswith("help"):
            [self.log_message(LogLevel.INFO, line) for line in self.HELP_LINES]

        else:
            self.log_message(LogLevel.ERROR, "Command not recognised")

    def _do_cyclic_task(self):
        if self.is_rtt_enabled:
                try:
                    with self.rtt_lock:
                        output = self.jlink.rtt_read(0, 1024, encoding=None)
                    if output:
                        output = self.elfparser.feed_with_bytes(output)
                        if output:
                            self._handle_parsed_input_from_backend_medium(output)
                except API.APIError:
                    self.log_message(LogLevel.ERROR, "RTT device read failed. Probably board reset will be needed.")
                    self.jlink.close()
                    self.is_rtt_enabled = False

    def _gracefully_shutdown(self):
        if self.is_rtt_enabled:
            self.jlink.close()