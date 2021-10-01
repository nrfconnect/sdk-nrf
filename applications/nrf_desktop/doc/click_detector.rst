.. _nrf_desktop_click_detector:

Click detector module
#####################

The click detector module is responsible for sending a ``click_event`` when a known type of click is recorded for the button defined in the module configuration.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_click_detector_start
    :end-before: table_click_detector_end

.. note::
    |nrf_desktop_module_event_note|

Implementation details
**********************

For implementation details refer to :ref:`caf_click_detector`.
