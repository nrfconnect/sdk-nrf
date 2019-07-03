from getch import _filter_char

class ZephyrLogParserShell:
    def __init__(self, output_function, input_parser_function, prompt=">> "):
        self.output = output_function
        self.input_parser = input_parser_function
        self.prompt = prompt
        self.current_user_input = self.prompt

    def handle_new_line(self, line):
        self.clear_line()
        self.output("\r{}\n".format(line))
        self.output(self.current_user_input)

    def handle_input_char(self, char):
        char = _filter_char(char)
        if char == '\b' or char == '\x7f':  # backspace
            if len(self.current_user_input) > len(self.prompt):
                self.current_user_input = self.current_user_input[:-1]
                self.clear_last_char()
        else:
            self.append_to_console_output(char)
            if char == '\n' or char == '\r':
                command = self.current_user_input[len(self.prompt):]
                self.input_parser(command)
                self.clear_line()
                self.append_to_console_output(self.prompt)
                self.current_user_input = self.prompt
            else:
                self.current_user_input += char

    def clear_line(self):
        self.output("\r{}\r".format(" " * len(self.current_user_input)))

    def append_to_console_output(self, string):
        self.output(string)

    def clear_last_char(self):
        self.output("\b \b")
