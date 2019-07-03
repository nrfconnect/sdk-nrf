from elftools.elf.elffile import ELFFile
import re
import datetime

_ANSI_COLOR_BLACK       = '\x1b[30m'
_ANSI_COLOR_RED         = '\x1b[31m'
_ANSI_COLOR_GREEN       = '\x1b[32m'
_ANSI_COLOR_YELLOW      = '\x1b[33m'
_ANSI_COLOR_BLUE        = '\x1b[34m'
_ANSI_COLOR_MAGENTA     = '\x1b[35m'
_ANSI_COLOR_LIGHTBLUE   = '\x1b[36m'
_ANSI_COLOR_WHITE       = '\x1b[37m'

_ANSI_COLOR_RESET = '\x1b[0m'

_COLOR_PARSED_LOGS_SEVERITY = False

class ZephyrElfStringParser:

    SEVERITY_STRINGS =          ["None", "Error", "Warning", "Info", "Debug"]
    SEVERITY_STRINGS_COLORS =   {"None":    _ANSI_COLOR_WHITE,
                                 "Error":   _ANSI_COLOR_RED,
                                 "Warning": _ANSI_COLOR_YELLOW,
                                 "Info":    _ANSI_COLOR_GREEN,
                                 "Debug":   _ANSI_COLOR_MAGENTA}

    LOG_TYPE_HEXDUMP = 1
    LOG_TYPE_FORMATTED_STRING = 2
    LOG_TYPE_PRINTK = 3
    LOG_TYPE_SHELL = 4

    STATE_EMPTY = 1
    STATE_HEXDUMP = 2
    STATE_HEXDUMP_COLLECTING_BYTES = 3
    STATE_PRINTK = 4
    STATE_PRINTK_COLLECTING_BYTES = 5
    STATE_FORMATTED_STRING = 6
    STATE_WAITING_FOR_SUFFIX = 7
    STATE_WAIT_FOR_SYNC = 8

    SYNC_FRAME = [0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68]

    def __init__(self, elf_file_path, use_internal_timestamp, sections_to_import=("log_const_sections", "rodata")):
        fd = open(elf_file_path, 'rb')
        elf = ELFFile(fd)
        self.sections= {}
        for section in elf.iter_sections():
            if section.name in sections_to_import:
                mysection = {}
                mysection["data"] = section.data()
                mysection["sh_addr"] = section.header["sh_addr"]
                mysection["sh_size"] = section.header["sh_size"]
                self.sections[section.name] = mysection
        fd.close()

        self.byte_accumulator = []
        self.state = self.STATE_EMPTY
        self.data = []
        self.args_amount = 0
        self.final_string = ""
        self.parsed_strings = []
        self.use_internal_timestamp = use_internal_timestamp
        self.internal_timestamp = ""
        self.last_eight_bytes = [0] * 8
        self.log_type = 0


    def two_bytes_to_uint16(self, arr):
        # assumes LITTLE ENDIAN format in byte_arr
        return arr[0] | arr[1] << 8

    def four_bytes_to_uint32(self, arr):
        # assumes LITTLE ENDIAN format in byte_arr
        return arr[0] | arr[1]<<8 | arr[2]<<16 | arr[3]<<24

    def get_section_offset(self, section):
        return self.sections[section]['sh_addr']

    def get_section_name_from_address(self, address):
        for section_name, section in self.sections.items():
            start_addr = section['sh_addr']
            end_addr = section['sh_size'] + start_addr
            if start_addr <= address < end_addr:
                return section_name
        return ""

    def get_uint32_from_address(self, address):
        byte_arr = self.get_data_chunk(address)[:4]
        return self.four_bytes_to_uint32(byte_arr)

    def get_data_chunk(self, address):
        name = self.get_section_name_from_address(address)
        offset = self.get_section_offset(name)
        chunk = self.sections[name]["data"][address-offset:]
        return chunk

    def is_ascii_printable(self, number):
        if ord(' ') <= number <= ord('~'):
            return True
        else:
            return False

    def int_to_char(self, number):
        if self.is_ascii_printable(number):
            return chr(number)
        else:
            return 0

    def get_arg_types_in_string(self, string):
        thisre = "[^%]%[a-z]"
        matches = re.findall(thisre, string)
        return [item[-1:] for item in matches]

    def get_string(self, address):
        if isinstance(address, str):
            addr_numeric = int(address, 16)
        elif isinstance(address, int):
            addr_numeric = address
        else:
            print("Wrong hexstring address format")
            return

        try:
            data = self.get_data_chunk(addr_numeric)
        except KeyError:
            return ""
        i = 0
        char = self.int_to_char(data[i])
        string = ""
        while char:
            string += char
            i += 1
            char = self.int_to_char(data[i])
        return string

    def get_formatted_string(self, *args):
        string_addr = args[0]
        arguments = args[1:]
        string = self.get_string(string_addr)
        return string % arguments

    def is_hexdump_msg(self, value):
        return value == 0xFFFFFFFF

    def is_printk_msg(self, value):
        return value == 0xFEFFFFFF

    def _pop_from_byte_accumulator(self, bytes_to_pop):
        if len(self.byte_accumulator) < bytes_to_pop:
            popped_bytes = []
        else:
            popped_bytes = self.byte_accumulator[:bytes_to_pop]
            self.byte_accumulator = self.byte_accumulator[bytes_to_pop:]
        return popped_bytes

    def _pop_from_byte_accumulator2(self, bytes_to_pop):
        popped_bytes = []
        if len(self.byte_accumulator) >= bytes_to_pop:
            [popped_bytes.append(self.byte_accumulator.pop()) for _ in range(bytes_to_pop)]
        return popped_bytes

    def _get_internal_timestamp(self):
        return datetime.datetime.now().strftime("%H:%M:%S.%f")

    def get_log_severity_level(self, suffix):
        severity_id = suffix & (0x07)
        try:
            severity_string = self.SEVERITY_STRINGS[severity_id]
        except IndexError:
            severity_string = "None"
        return severity_string

    def get_log_severity_ansi_color(self, severity):
        return self.SEVERITY_STRINGS_COLORS[severity]

    def get_log_source_module(self, suffix):
        module_id = (suffix >> 6)
        try:
            address_of_string_addr = self.get_section_offset('log_const_sections') + (8 * module_id)
            module_string_addr = self.get_uint32_from_address(address_of_string_addr)
            module_string = self.get_string(module_string_addr)
        except (IndexError, ValueError):
            module_string = "None"
        return module_string

    def wait_for_sync_frame(self):
        self._reset_state_machine()
        self.state = self.STATE_WAIT_FOR_SYNC

    def _add_to_history(self, bytes):
        for byte in bytes:
            self.last_eight_bytes.pop(0)
            self.last_eight_bytes.append(byte)
            self._check_history()

    def _check_history(self):
        if self.last_eight_bytes == self.SYNC_FRAME:
            self._reset_state_machine()

    def _reset_state_machine(self):
        self.state = self.STATE_EMPTY
        self.data = []
        self.args_amount = 0
        self.log_type = 0

    def feed_with_bytes(self, bytes):
        self.byte_accumulator += bytes

        is_enough_bytes = True
        while is_enough_bytes:
            if self.state == self.STATE_EMPTY:
                four_bytes = self._pop_from_byte_accumulator(4)
                if four_bytes:
                    if self.use_internal_timestamp:
                        self.internal_timestamp = self._get_internal_timestamp()
                    string_addr = self.four_bytes_to_uint32(four_bytes)
                    if self.is_hexdump_msg(string_addr):
                        self.state = self.STATE_HEXDUMP
                        self.log_type = self.LOG_TYPE_HEXDUMP
                    elif self.is_printk_msg(string_addr):
                        self.state = self.STATE_PRINTK
                        self.log_type = self.LOG_TYPE_PRINTK
                    else:
                        self.state = self.STATE_FORMATTED_STRING
                        self.log_type = self.LOG_TYPE_FORMATTED_STRING
                        string = self.get_string(string_addr)
                        if string == "":
                            popped_byte = four_bytes[0]
                            self.byte_accumulator = four_bytes[1:] + self.byte_accumulator
                            self.state = self.STATE_EMPTY
                            self._add_to_history([popped_byte])
                            continue
                        self.args_amount = len(self.get_arg_types_in_string(string))
                        self.data.append(string_addr)
                    self._add_to_history(four_bytes)
                else:
                    is_enough_bytes = False

            elif self.state == self.STATE_HEXDUMP:
                four_bytes = self._pop_from_byte_accumulator(4)
                if four_bytes:
                    self.args_amount = self.four_bytes_to_uint32(four_bytes)
                    self.state = self.STATE_HEXDUMP_COLLECTING_BYTES
                    self._add_to_history(four_bytes)
                else:
                    is_enough_bytes = False

            elif self.state == self.STATE_PRINTK:
                four_bytes = self._pop_from_byte_accumulator(4)
                if four_bytes:
                    self.args_amount = self.four_bytes_to_uint32(four_bytes)
                    self.state = self.STATE_PRINTK_COLLECTING_BYTES
                    self._add_to_history(four_bytes)
                else:
                    is_enough_bytes = False

            elif self.state == self.STATE_HEXDUMP_COLLECTING_BYTES:
                if self.args_amount > len(self.data):
                    byte = self._pop_from_byte_accumulator(1)
                    if byte:
                        self.data.append("{0:#0x}".format(byte[0]))
                        self._add_to_history(byte)
                    else:
                        is_enough_bytes = False
                else:
                    self.final_string = " ".join(self.data)
                    self.state = self.STATE_WAITING_FOR_SUFFIX

            elif self.state == self.STATE_PRINTK_COLLECTING_BYTES:
                if self.args_amount > len(self.data):
                    byte = self._pop_from_byte_accumulator(1)
                    if byte:
                        self.data.append(chr(byte[0]))
                        self._add_to_history(byte)
                    else:
                        is_enough_bytes = False
                else:
                    self.final_string = "".join(self.data)
                    parsed_string = "<{}> {}".format("PRINTK", self.final_string)
                    if self.use_internal_timestamp:
                        parsed_string = "[{}] {}".format(self.internal_timestamp, parsed_string)
                    self.parsed_strings.append([self.log_type, parsed_string])
                    self._reset_state_machine()

            elif self.state == self.STATE_FORMATTED_STRING:
                if self.args_amount > len(self.data[1:]):
                    four_bytes = self._pop_from_byte_accumulator(4)
                    if four_bytes:
                        this_u32_t = self.four_bytes_to_uint32(four_bytes)
                        self.data.append(this_u32_t)
                        self._add_to_history(four_bytes)
                    else:
                        is_enough_bytes = False
                else:
                    try:
                        self.final_string = self.get_formatted_string(*self.data)
                    except ValueError:
                        self.final_string = self.get_string(self.data[0]) + ", ".join(self.data[1:])
                    self.state = self.STATE_WAITING_FOR_SUFFIX

            elif self.state == self.STATE_WAIT_FOR_SYNC:
                eight_bytes = self._pop_from_byte_accumulator(8)
                if eight_bytes:
                    if eight_bytes == self.SYNC_FRAME:
                        self._reset_state_machine()
                    else:
                        self.byte_accumulator = eight_bytes[1:] + self.byte_accumulator
                else:
                    is_enough_bytes = False

            if self.state == self.STATE_WAITING_FOR_SUFFIX:
                two_bytes = self._pop_from_byte_accumulator(2)
                if two_bytes:
                    suffix = self.two_bytes_to_uint16(two_bytes)
                    severity = self.get_log_severity_level(suffix)
                    module = self.get_log_source_module(suffix)
                    parsed_string  = "<{:7}> <{}> {}".format(severity, module , self.final_string)
                    if self.use_internal_timestamp:
                        parsed_string = "[{}] {}".format(self.internal_timestamp, parsed_string)
                    if _COLOR_PARSED_LOGS_SEVERITY:
                        color = self.get_log_severity_ansi_color(severity)
                        parsed_string = color + parsed_string + _ANSI_COLOR_RESET
                    self.parsed_strings.append([self.log_type, parsed_string])
                    self._reset_state_machine()
                    self._add_to_history(two_bytes)
                else:
                    is_enough_bytes = False

        retval = self.parsed_strings
        self.parsed_strings = []
        return retval
