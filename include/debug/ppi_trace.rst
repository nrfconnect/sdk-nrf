.. _ppi_trace:

PPI trace
#########

.. contents::
   :local:
   :depth: 2

The PPI trace module enables tracing of hardware peripheral events on pins.
Tracing is performed without CPU intervention, because PPI is used to connect events with tasks in GPIOTE.

PPI trace can be used to debug a single event or a pair of complementary events.
When tracing a single event, every occurrence of the event toggles the state of the pin (see :c:func:`ppi_trace_config`).
When tracing a pair of complementary events (for example, the start and end of a transfer), the pin is set when one of the events occurs and cleared when the other event occurs (see :c:func:`ppi_trace_pair_config`).

The PPI trace module is used in the :ref:`ppi_trace_sample` sample.

API documentation
*****************

| Header file: :file:`include/debug/ppi_trace.h`
| Source files: :file:`subsys/debug/ppi_trace/`

.. doxygengroup:: ppi_trace
   :project: nrf
   :members:
