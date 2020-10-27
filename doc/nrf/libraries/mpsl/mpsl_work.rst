.. _mpsl_work:

Multiprotocol Service Layer workqueue
#####################################

.. contents::
   :local:
   :depth: 2

The Multiprotocol Service Layer (MPSL) workqueue library allows submitting tasks to the :ref:`nrfxlib:mpsl` workqueue for processing.
The MPSL workqueue is a workqueue running with a higher priority than the system workqueue.

Implementation
**************

The MPSL workqueue is an instance of the Zephyr kernel workqueue which processes the MPSL low-priority signals.

Usage
*****

The MPSL workqueue is intended to be used by callers of the low-priority MPSL API to process work in the same thread as the MPSL low-priority signals.

API documentation
*****************

| Header file: :file:`include/mpsl/mpsl_work.h`
| Source files: :file:`subys/mpsl/`

.. doxygengroup:: mpsl_work
   :project: nrf
   :members:
