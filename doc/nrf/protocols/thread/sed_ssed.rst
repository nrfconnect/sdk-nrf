.. _thread_sed_ssed:

Sleepy End Device types in Thread
#################################

Sleepy End Devices (SEDs) and Synchronized Sleepy End Devices (SSEDs) are both Minimal Thread Devices (MTDs).
Unlike Full Thread Devices (FTDs), MTDs do not maintain a routing table and are typically low-power devices that are not always on.

For more information, see the :ref:`thread_types_mtd` section in the :ref:`thread_device_types` documentation.

Sleepy End Device
*****************

SEDs are MTDs that sleep most of the time in order to minimize the power consumption.
They communicate with the Thread network by occasionally polling the parent Router for any pending data.

The :kconfig:option:`CONFIG_OPENTHREAD_POLL_PERIOD` Kconfig handles the SED configuration by configuring the polling period.
A higher polling frequency results in lower latency (better responsiveness), but also higher power consumption.

The polling period can also be configured in runtime.
See the ``pollperiod`` command in the OpenThread `CLI reference <OpenThread CLI Reference - pollperiod command>`_.

Synchronized Sleepy End Device
******************************

SSEDs are MTDs that are further optimized for power consumption.
A Thread SSED is synchronized with its parent Router and uses the radio only at scheduled intervals.
It does this by using the :ref:`thread_ug_supported_features_csl` feature (introduced as one of the `Thread 1.2 Base Features`_).
During those intervals, the device waits for the Router to send it any data related to the desired device activity.
This reduces the network traffic (since there is no polling) and the power consumption (since the radio is off most of the time).

An SSED does require sending packets occasionally to keep synchronization with the Router.
However, unlike a regular SED, an SSED does not actively communicate with the Router by polling and it goes into the idle mode between the scheduled activity periods.
If there is no application-related traffic for an extended period of time, the SSED sends a data poll request packet to synchronize with the parent.

The |NCS| provides the following Kconfig options that let you enable CSL and specify the CSL parameters:

* :kconfig:option:`CONFIG_OPENTHREAD_CSL_RECEIVER` - Enables SSED child mode.
* :kconfig:option:`CONFIG_OPENTHREAD_CSL_AUTO_SYNC` - Enables the CSL autosynchronization feature.
* :kconfig:option:`CONFIG_OPENTHREAD_CSL_TIMEOUT` - Sets the default CSL timeout in seconds.
* :kconfig:option:`CONFIG_OPENTHREAD_CSL_CHANNEL` - Sets the default CSL channel.
  This option corresponds to the ``csl channel`` CLI parameter.

The following Kconfig options affect the size of the receive window, and thus also affect the device's power consumption:

* :kconfig:option:`CONFIG_OPENTHREAD_CSL_RECEIVE_TIME_AHEAD` - Sets the CSL receiver wake up margin in microseconds.
* :kconfig:option:`CONFIG_OPENTHREAD_MIN_RECEIVE_ON_AHEAD` - Sets the minimum receiving time before start of MAC header.
* :kconfig:option:`CONFIG_OPENTHREAD_MIN_RECEIVE_ON_AFTER` - Sets the minimum receiving time after start of MAC header.
* :kconfig:option:`CONFIG_OPENTHREAD_PLATFORM_CSL_UNCERT` - Sets the fixed uncertainty of the device for scheduling CSL Transmissions in units of 10 microseconds.

Additionally, you must configure the ``period`` CLI parameter to enable CSL.
For more information on using the CLI to configure parameters for CSL, see the ``csl`` command in the OpenThread `CLI reference <OpenThread CLI Reference - csl command_>`_.

Comparison of SED and SSED
**************************

Compared to an SED, an SSED has no drawbacks for transmission and provides reduced power consumption and network traffic.
This means you should configure your SED devices as SSEDs whenever possible.
You can see the difference in power consumption on the :ref:`Thread power consumption <thread_power_consumption>` page.

.. figure:: overview/images/thread_sed_ssed_comparison.svg
   :alt: Comparison of Thread SED and Thread SSED radio activity

   Comparison of Thread SED and Thread SSED radio activity
