.. _at_params_readme:

AT parameters
#############

The AT parameters module provides functionality to store lists of AT command or respond parameters.
These lists can be used by other modules to write and read parameter values.

.. tip::
   The primary intention of this module is to be used for AT command or respond parameters, but this is no requirement.
   You can use this module to store other kinds of parameters, as long as they consist of numeric or string values.

A parameter list contains an array of parameters defined by a type and a value.
It can be cleared and reused, but once a list is created, its size cannot be changed.
Each parameter in a list can be updated or cleared.
When setting a parameter in the list, its value is copied.
Parameters should be cleared to free the memory that they occupy.
Getter and setter methods are available to read parameter values.


API documentation
*****************

| Header file: :file:`include/at_params.h`
| Source file: :file:`lib/at_cmd_parser/src/at_params.c`

.. doxygengroup:: at_params
   :project: nrf
   :members:
