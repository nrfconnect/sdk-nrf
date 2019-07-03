import intelhex
import re

class IntelHexStringParser:
    def __init__(self, hexfilepath):
        self.hexobj = intelhex.IntelHex()
        self.hexobj.loadhex(hexfilepath)

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
        char = self.int_to_char(self.hexobj[addr_numeric])
        string = ""
        while char:
            string += char
            addr_numeric += 1
            char = self.int_to_char(self.hexobj[addr_numeric])
        return string

    def get_formatted_string(self, *args):
        string_addr = args[0]
        arguments = args[1:]
        string = self.get_string(string_addr)
        return string % arguments