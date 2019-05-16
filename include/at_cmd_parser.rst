.. _at_cmd_parser_readme:

AT command parser
#################

The AT command parser is a library for parsing the string that is returned after issuing an AT command.
It parses the response parameters from the string and saves them in a list.

To store the parameters in a list, the parser uses the :ref:`at_params_readme` storage module.
The resulting list can be used by any external module to obtain the value of the parameters.
Based on the type of the parameter, you can obtain its value by calling either :cpp:func:`at_params_int_get`, :cpp:func:`at_params_short_get`, or :cpp:func:`at_params_string_get`.

Before using the AT command parser, you must initialize a list of AT command/response parameters by calling :cpp:func:`at_params_list_init`.
Then, to parse a string, simply pass the returned AT command string to the library function :cpp:func:`at_parser_params_from_str`.


API documentation
*****************

| Header file: :file:`include/at_cmd_parser.h`
| Source file: :file:`lib/at_cmd_parser/src/at_cmd_parser.c`

.. doxygengroup:: at_cmd_parser
   :project: nrf
   :members:
