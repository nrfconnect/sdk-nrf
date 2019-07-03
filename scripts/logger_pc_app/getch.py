import platform

def _filter_char(char_to_filter):
    char = char_to_filter
    if len(char) != 1:  # drop special characters
        char = ""
    elif char == '\x03':  # ctrl+c
        raise KeyboardInterrupt
    elif ord(char) == 127 or char == '\b':  # On Linux DEL is returned when backspace is pressed
        char = '\b'
    elif char == '\n' or char == '\r':  # Handle line breaks
        pass
    elif not 31 < ord(char) < 123:  # finally if not printable
        char = ""
    return char


def getChar():
    # figure out which function to use once, and store it in _func
    if "_func" not in getChar.__dict__:
        if platform.system() == "Windows":
            import msvcrt

            def _winRead():
                char = msvcrt.getwch()
                return _filter_char(char)

            getChar._func=_winRead

        else:
            import tty, sys, termios # raises ImportError if unsupported

            def _ttyRead():
                fd = sys.stdin.fileno()
                oldSettings = termios.tcgetattr(fd)

                try:
                    tty.setcbreak(fd)
                    answer = sys.stdin.read(1)
                finally:
                    termios.tcsetattr(fd, termios.TCSADRAIN, oldSettings)

                return _filter_char(answer)

            getChar._func=_ttyRead

    return getChar._func()
