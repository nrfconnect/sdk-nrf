.. _at_cmd_parser_readme:

AT command parser
#################

The AT command parser is a library which parses any of the following:

- A string response returned after issuing an AT command
- An unsolicited event
- A notification

After parsing the response, notification, or event, the AT command parser extracts the parameters from the string and saves them in a structured list.

The :ref:`at_params_readme` storage module is used to store the parameters.
Each element in the list is a parameter, identified by a specific type, length, and value.
The resulting list can be used by any external module to obtain the value of the parameters.
Based on the type of the parameter, you can obtain its value by calling :cpp:func:`at_params_int_get`, :cpp:func:`at_params_short_get`, :cpp:func:` at_params_array_get`, or :cpp:func:`at_params_string_get`.
The function :cpp:func:`at_params_size_get` can be used to read the length of a parameter or to check if a parameter exists at a specified index of the list.
Probing which type of element is stored on which index can be done using the :cpp:func:`at_params_type_get`.

Before using the AT command parser, you must initialize a list of AT command/response parameters by calling :cpp:func:`at_params_list_init`.
Then, to parse a string, simply pass the returned AT command string to the library function :cpp:func:`at_parser_params_from_str`.


API documentation
*****************

| Header file: :file:`include/at_cmd_parser.h`
| Source file: :file:`lib/at_cmd_parser/src/at_cmd_parser.c`

.. doxygengroup:: at_cmd_parser
   :project: nrf
   :members:
