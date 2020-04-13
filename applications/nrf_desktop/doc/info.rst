.. _nrf_desktop_info:

Info module
###########

The Info module is required by the :ref:`nrf_desktop_config_channel`.
The module is the final subscriber for the :ref:`nrf_desktop_config_channel` events and provides the board name through the :ref:`nrf_desktop_config_channel`.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_info_start
    :end-before: table_info_end

See the :ref:`nrf_desktop_architecture` for more information about the event-based communication in the nRF Desktop application and about how to read this table.

Configuration
*************

The module is enabled with the same Kconfig option as the :ref:`nrf_desktop_config_channel`: ``CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE``.

Implementation details
**********************

The Info module must be the final subscriber for ``config_event`` and ``config_fetch_request_event``.
Otherwise some listeners may not be accessible by the :ref:`nrf_desktop_config_channel_script`.

The board name provided by the module through the :ref:`nrf_desktop_config_channel` is a part of the Zephyr board name (:option:`CONFIG_BOARD`), beginning with the predefined prefix (:c:macro:`BOARD_PREFIX`).
