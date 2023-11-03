.. _fault_handling:

Modem fault handling
####################

.. contents::
   :local:
   :depth: 2

The application core and the modem core are separate cores on the nRF91 Series SiPs.
When the modem core on an nRF91 Series SiP crashes, it sends a fault signal to the application core.

Detecting a fault
*****************

The Modem library implements a fault handling mechanism, allowing the application to be notified when the modem core has crashed.
The application can set a modem fault handler function using the Modem library initialization parameters during initialization.
When the modem crashes, the modem fault handler set by the application is called with the fault reason.
In some cases, the modem fault handler also contains information about the program counter of the modem.
For a complete list of the modem fault reasons, see :ref:`nrf_modem_fault`.

In |NCS|, the :ref:`nrf_modem_lib_readme` sets the modem fault handler during Modem library initialization.

Recovering from a fault
***********************

When the modem has crashed, it can be reinitialized by the application.
The application core does not need to be reset to recover the modem from a fault state.
The application can reinitialize the modem by reinitializing the Modem library through calls to the :c:func:`nrf_modem_shutdown` and :c:func:`nrf_modem_init` functions.

.. important::

   The reinitialization of the Modem library must be done outside of the Modem library fault handler.

.. important::
   If modem traces are enabled, the modem continues to output trace data (a coredump) in the event of a fault, even after signaling the fault to the application.
   To make sure the application is able to retrieve the coredump correctly, the application must not re-initialize the modem until all the outstanding trace data has been processed.
   The :c:func:`nrf_modem_trace_get()` function will return ``-ENODATA`` when all outstanding trace data has been processed by the application.

When the Modem library is used in |NCS|, the :ref:`nrf_modem_lib_readme` handles synchronizing modem re-initialization with tracing operations.

Networking sockets
******************

Socket APIs that require communication with the modem, return ``-1`` and set ``errno`` to ``NRF_ESHUTDOWN`` if the modem has crashed or if it is uninitialized.

Although the modem has crashed, any data which was stored by the Modem library, including data that was delivered to the Modem library by the modem, remains available until the Modem library is shut down.
This includes incoming network data, which was received before the crash but has not been read by the application.
The application can read that data as normal, using the :c:func:`recv` function.
When no more data is available, the :c:func:`recv` function returns ``-1`` and sets ``errno`` to ``NRF_ESHUTDOWN``.

Ongoing API calls
*****************

The modem can crash when a function call in Modem library is ongoing, for example, a socket API call.
When the Modem library detects that a fault has occurred in the modem, it immediately exits all calls that are waiting for a modem response.

Following are the two categories of APIs in the Modem library and their behavior after a modem fault:

* Socket APIs - Exit immediately returning ``-1`` and sets errno to ``NRF_ESHUTDOWN``
* Non-Socket APIs - Exit immediately returning ``-NRF_ESHUTDOWN``
