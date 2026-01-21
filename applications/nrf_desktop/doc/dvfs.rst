.. _nrf_desktop_dvfs:

DVFS module
###########

.. contents::
   :local:
   :depth: 2

The nRF Desktop Dynamic Voltage and Frequency Scaling (DVFS) module is responsible for switching the frequency and voltage according to the application needs.
The module chooses the highest frequency required by the application states.
The module, requests this frequency as a minimal requirement to clock control driver which selects DVFS operating point accordingly.
DVFS improves balancing performance and power consumption, especially in battery-operated devices.

In the nRF Desktop application, the DVFS module enables the system to use the lowest possible frequency and voltage when the application performs less compute-intensive activities, such as Bluetooth® LE advertising.
However, if performing more compute-intensive tasks, the nRF Desktop DVFS module will dynamically change to a higher frequency to increase performance.

The nRF Desktop DVFS module requests clock frequency change using API of the clock control driver.
The driver is responsible for choosing the optimal clock frequency.
The driver submits a request to the sysctrl core through the nrfs DVFS service.
The sysctrl core controls both clock frequency and voltage and chooses corresponding voltage to requested frequency.

.. note::
   In the current solution, it is assumed that no other module requests frequency changes.
   The nRF Desktop DVFS module is the sole entity responsible for managing frequency and voltage adjustments.
   Controlling clock from other sources would result in a module fatal error.

Supported devices
*****************

The module is only available for the nRF54H20 SoC.

The nRF54H20 SoC has the following available operating points (called frequency in the documentation):

* DVFS_FREQ_HIGH - High frequency - 320 MHz, 0.8 V
* DVFS_FREQ_MEDLOW - Medium-low frequency - 128 MHz, 0.6V
* DVFS_FREQ_LOW - Low frequency - 64 MHz, 0.5V

The nRF Desktop DVFS module might request any clock frequency, but it would be rounded up by the driver to one of the values from available operating points.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_dvfs_start
    :end-before: table_dvfs_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The nRF Desktop DVFS module is enabled by default on supported board targets.
To disable it, set the :option:`CONFIG_DESKTOP_DVFS` Kconfig option to ``n``.

Tracked application states
==========================

The nRF Desktop DVFS module defines independent application states that have different requirements in terms of frequency.
The module listens for Event Manager events that are sent and turns the state on or off according to the event.
It support the following DVFS states:

* ``DVFS_STATE_INITIALIZING`` - Application initialization is in progress
* ``DVFS_STATE_LLPM_CONNECTED`` - There is an ongoing connection that uses BLE Low Latency Packet Mode
* ``DVFS_STATE_USB_CONNECTED`` - USB is connected
* ``DVFS_STATE_CONFIG_CHANNEL`` - :ref:`nrf_desktop_config_channel` is active
* ``DVFS_STATE_SMP_TRANSFER`` - DFU image transfer over BLE SMP is active
  For details about SMP DFU integration in nRF Desktop, see :ref:`nrf_desktop_dfu_mcumgr`.

You can configure each DVFS state using the following Kconfig options:

* ``CONFIG_DESKTOP_DVFS_STATE_<STATE>_ENABLE`` - Enables DVFS state <STATE> handling.
* ``CONFIG_DESKTOP_DVFS_STATE_<STATE>_ACTIVE_FREQ`` - Choice of minimal active frequency requested by state <STATE>.
  The possible values are ``DESKTOP_DVFS_STATE_<STATE>_ACTIVE_FREQ_HIGH`` and ``DESKTOP_DVFS_STATE_<STATE>_ACTIVE_FREQ_MEDLOW``.
* ``CONFIG_DESKTOP_DVFS_STATE_<STATE>_TIMEOUT_MS`` - Specifies the timeout in milliseconds, defining the duration from receiving an on event to the moment the state <STATE> stops requesting its active frequency.
  When the timeout is reached, the state returns to a non-active state potentially enabling the system to use lower frequency.
  If the timeout is set to ``0`` the <STATE> will continue requesting its active frequency until it receives an off event.
  Some of the tracked application states have associated events that inform when the state is turned on and off (for example, USB connection).
  Other states have associated application events emitted periodically when active.
  There is no event explicitly informing that state is no longer active (for example, config channel).
  For such states, you need to define a timeout that turns off the tracked state after associated application events are no longer emitted.

Frequency change retry
======================

Each frequency change request is retried if it fails due to a known reason.
Failing frequency change requests can be caused by:

* Another frequency change request being in progress on sysctrl - error ``-EBUSY``.
  In that case, the module will retry the request after a timeout specified by the :option:`CONFIG_DESKTOP_DVFS_RETRY_BUSY_TIMEOUT_MS` Kconfig option.
  The default timeout value is 1 ms.
* The nrfs DVFS service being not yet initialized - error ``-EAGAIN``.
  In that case, the nRF Desktop DVFS module will retry the request after a timeout specified by the :option:`CONFIG_DESKTOP_DVFS_RETRY_INIT_TIMEOUT_MS` Kconfig option.
  The default timeout value is 500 ms.
* Other errors are not retried.
  If the DVFS frequency request returns an error different from ``-EBUSY`` or ``-EAGAIN``, the module will report a :c:enum:`MODULE_STATE_ERROR` and set the ``module_state`` to ``STATE_ERROR``.

The module will retry the request up to the number of times specified by the :option:`CONFIG_DESKTOP_DVFS_RETRY_COUNT` Kconfig option.
The default value of retries is ``5``.
The error counter is only set to zero when the DVFS change request is correct.
nRF Desktop DVFS module will also report a :c:enum:`MODULE_STATE_ERROR` and set ``module_state`` to ``STATE_ERROR`` if the number of retries is exceeded.

In ``STATE_ERROR``, the module will not change the frequency and voltage.
:c:enum:`MODULE_STATE_ERROR` reported by the nRF Desktop DVFS module would lead to fatal application error.

Implementation details
**********************

The nRF Desktop DVFS module has the following two state tracing entities:

* ``module_state`` is used to track the state of the nRF Desktop DVFS module.
  It indicates if the module is ready or if an error has occurred.

* ``dfvs_requests_state_bitmask`` is used to track separately the state of each application DVFS state.
  Each DVFS state is represented by a bit in the bitmask.
  If the bit is set, the state is active.
  The module listens for Event Manager events related to a given state and updates the state accordingly.
  The module then checks which states are active and sets the highest frequency that is required by at least one state.
  If no state is active, the module sets the lowest frequency :option:`CONFIG_DESKTOP_DVFS_FREQ_LOW`.
