.. _nrf_desktop_dvfs:

DVFS module
###########

.. contents::
   :local:
   :depth: 2

The Dynamic Voltage and Frequency Scaling (DVFS) module is responsible for switching the frequency and voltage according to the application needs.
DVFS is critical for balancing performance and power consumption, especially in battery-operated devices.

In the nRF Desktop, the DVFS module enables the system to use the lowest possible frequency and voltage when using classic BLE as it is enough to run this mode.
However, if performing more computation-consuming tasks DVFS module will dynamically change to a higher frequency to increase performance.

The module is only available for nrf54h20 SoC.
Available operating points (in documentation called frequency):

1. DVFS_FREQ_HIGH - high frequency - 320 MHz, 0.8 V
#. DVFS_FREQ_MEDLOW - medium-low frequency - 128 MHz, 0.6V
#. DVFS_FREQ_LOW - low frequency - 64 MHz, 0.5V


Module events
*************

.. include:: event_propagation.rst
    :start-after: table_dvfs_start
    :end-before: table_dvfs_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The Dynamic Voltage and Frequency Scaling (DVFS) module is enabled by default for nrf54h20 Soc and can be disabled by setting the :option:`CONFIG_DESKTOP_DVFS` Kconfig option to ``n``.


The DVFS module can be configured using the following Kconfig options:

1. :option:`CONFIG_DESKTOP_DVFS_RETRY_BUSY_TIMEOUT_MS` - Timeout in milliseconds specifying time after which DVFS module will retry DVFS frequency change.
   This timeout is applied in case another DVFS change request is still in progress which causes the current request to fail.
   The default value is 1 ms.
#. :option:`CONFIG_DESKTOP_DVFS_RETRY_INIT_TIMEOUT_MS` - Timeout in milliseconds specifying time after which DVFS module will retry DVFS frequency change.
   This timeout is applied in case DVFS is not yet initialized which causes the current request to fail.
   The default value is 500 ms.
#. :option:`CONFIG_DESKTOP_DVFS_RETRY_COUNT` - Number of retries of DVFS frequency change after which DVFS module will report MODULE_STATE_ERROR.
   The default value is 5.

Each DVFS state can be configured using the following Kconfig options:

1. ``CONFIG_DESKTOP_DVFS_STATE_<STATE>_ENABLE`` - Enable DVFS state <STATE>.
#. ``CONFIG_DESKTOP_DVFS_STATE_<STATE>_TIMEOUT_MS`` - Timeout in milliseconds specifying time after which the DVFS module returns to low frequency after entering <STATE>.
#. ``CONFIG_DESKTOP_DVFS_STATE_<STATE>_ACTIVE_FREQ`` - Choice of active frequency of <STATE>.
    The possible values are:  ``DESKTOP_DVFS_STATE_<STATE>_ACTIVE_FREQ_HIGH`` and ``DESKTOP_DVFS_STATE_<STATE>_ACTIVE_FREQ_MEDLOW``.

Currently supported DVFS states are:

1. ``DVFS_STATE_INITIALIZING`` - State indicating that core initialization is in process.
#. ``DVFS_STATE_LLPM_CONNECTED`` - State indicating that Low Latency Packet Mode is active.
#. ``DVFS_STATE_USB_CONNECTED`` - State indicating that USB is connected.
#. ``DVFS_STATE_CONFIG_CHANNEL`` - State indicating that Config Channel is active.
#. ``DVFS_STATE_SMP_TRANSFER`` - State indicating that SMP transfer is active.

Implementation details
**********************

The Dynamic Voltage and Frequency Scaling (DVFS) has two state tracing entities.

First ``module_state`` is used to track the state of the DVFS module.
It indicates if the DVFS module is ready or if an error has occurred.

Second ``dfvs_requests_state_bitmask`` is used to track separately the state of each DVFS state.
Each DVFS state is represented by a bit in the bitmask.
If the bit is set, the state is active.
DVFS module listens for events that are sent and applies state activity according to the event.
DVFS module then checks which states are active and sets the highest frequency of all active states.
If no state is active, the module sets the lowest frequency: ``DVFS_FREQ_LOW``.

Each change request is retried if it fails.
The module will retry the request up to the number of times specified by :option:`CONFIG_DESKTOP_DVFS_RETRY_COUNT`.
Failing frequency change request can be caused by another frequency change request in progress error ``-EBUSY`` or by the DVFS module being not yet initialized error ``-EAGAIN``.
If the frequency change request is in progress, the module will retry the request after a timeout specified by :option:`CONFIG_DESKTOP_DVFS_RETRY_BUSY_TIMEOUT_MS`.
If the DVFS module is not yet initialized, the module will retry the request after a timeout specified by :option:`CONFIG_DESKTOP_DVFS_RETRY_INIT_TIMEOUT_MS`.
If the DVFS frequency request returns an error different than ``-EBUSY`` or ``-EAGAIN``, the module will report ``MODULE_STATE_ERROR`` and set ``module_state`` to ``STATE_ERROR``.
DVFS module will also report MODULE_STATE_ERROR and set ``module_state`` to ``STATE_ERROR`` if the number of retries exceeds the value specified by :option:`CONFIG_DESKTOP_DVFS_RETRY_COUNT`.
In ``module_state`` in ``STATE_ERROR`` the module will not change the frequency and voltage.

There is an option to set a timeout for each state.
If the timeout is reached, the state will go back to a nonactive state potentially enabling the system to go to lower frequency.
