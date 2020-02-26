.. _modem_info_readme:

Modem information
#################

The modem information library can be used by an LTE application to obtain specific data from a modem.
It issues AT commands to retrieve the following data:

* Signal strength indication (RSRP)
* Current and supported LTE bands
* Tracking area code, mobile country code, and mobile network code
* Current mode and operator
* The cell ID and IP address of the device
* UICC state, SIM ICCID and SIM IMSI
* The battery voltage and temperature level, measured by the modem
* The modem firmware version
* The modem serial number
* The LTE-M, NB-IoT, and GPS support mode
* Mobile network time and date

The modem information library uses the :ref:`at_cmd_parser_readme`.

Call :cpp:func:`modem_info_init` to initialize the library.
To obtain a data value, call :cpp:func:`modem_info_string_get` (to retrieve the value as a string) or :cpp:func:`modem_info_short_get` (to retrieve the value as a short).

You can also retrieve all available data.
To do so, call :cpp:func:`modem_info_params_init` to initialize a structure that stores all retrieved information, then populate it by calling :cpp:func:`modem_info_params_get`.
To retrieve the data as a single JSON string, call :cpp:func:`modem_info_json_string_encode`.

Note, however, that signal strength data (RSRP) is only available by registering a subscription. To do so, call :cpp:func:`modem_info_rsrp_register`.


API documentation
*****************

| Header file: :file:`include/modem/modem_info.h`
| Source files: :file:`lib/modem_info/`

.. doxygengroup:: modem_info
   :project: nrf
   :members:
