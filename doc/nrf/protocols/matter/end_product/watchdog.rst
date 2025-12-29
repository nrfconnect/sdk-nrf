.. _ug_matter_device_watchdog:

Matter watchdog
###############

.. contents::
   :local:
   :depth: 2

A watchdog is a type of hardware timer responsible for rebooting the device if it does not receive a specific signal within a designated time window.
This signal is referred to as a *feeding signal*, and sending the signal is referred to as *feeding the watchdog*.

In |NCS| Matter samples, you can create multiple watchdog sources and assign them to specific functions.
Each watchdog source created must be fed to reset the timer and prevent the device from rebooting.

Overview
********

The Matter watchdog is based on Zephyr's watchdog API and allows for the creation of multiple watchdog sources.
Each Matter watchdog source utilizes a unique Global watchdog channel and must be fed individually.
You can set a time window for the watchdog timer and enable the automatic feeding feature.

The feeding feature depends on establishing an additional timer that periodically executes the created feeding callback function.
You can create your own feeding callback function and use it to call the ``Feed()`` method in a specific context.
This approach eliminates the need to directly call the ``Feed()`` method in your code.

A time window specifies the period within which the feeding signal must be sent to each watchdog channel to reset the timer and prevent the device from rebooting.
If the feeding signal is sent after the time window has elapsed, it does not prevent the device from rebooting.

To enable the Matter watchdog feature in a Matter sample, set the :option:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG` Kconfig option to ``y``.
The feature is enabled by default for the release build type in all Matter samples and applications.

To set the timeout for the watchdog timer, configure the :option:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT` with a value in milliseconds.
By default, the timeout is set to 10 minutes.

.. note::

   The Matter watchdog implementation demonstrates how to use the Zephyr watchdog API and is specifically designed for Matter samples within the |NCS|.
   If you want to use it in your application, refer to the source code and Kconfig options from this implementation.

.. _ug_matter_device_watchdog_pause_mode:

Matter watchdog pause mode
**************************

You can set the Matter watchdog to pause when the CPU is in idle (sleep) mode or when the CPU is halted by the debugger.
This means the watchdog timer will stop counting down when the CPU is idle or halted.

If pause mode is disabled, the watchdog must be fed within a specified time window, regardless of the CPU state, otherwise the device will reboot.
If pause mode is enabled, the device will only reboot if the task feeding the watchdog is blocked for longer than the set time window.
Pause mode is less accurate and may cause the device to reboot later than expected, but it allows feeding the watchdog only when the CPU is active.
Non-pause mode requires periodic feeding to avoid a timeout, impacting power consumption as the CPU must wake up periodically.

To enable pause mode while CPU is in the idle state, set the :option:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_IN_SLEEP` Kconfig option to ``y``.
To enable pause mode during debugging, set the :option:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_PAUSE_ON_DEBUG` Kconfig option to ``y``.

By default, Matter samples enable the pause mode only during debugging.

Creating a Matter watchdog source
*********************************

The Matter watchdog feature is based on the ``Nrf::WatchdogSource`` class, which is located in the :file:`samples\matter\common\src\watchdog\watchdog.h` file.
Each Matter watchdog source constructor includes two optional arguments:

* ``feedingInterval`` - Specifies the duration in milliseconds for automatically calling the attached feeding callback.
* ``callback`` - A ``FeedingCallback`` implementation that is called periodically at the defined ``feedingInterval``.

After creating a watchdog source, you need to install it in the global watchdog module to assign a watchdog channel.
To do install it, call the ``InstallSource(WatchdogSource &source)`` function, provide the created source and check that the result of the function is ``true``.
After this, enable the global watchdog by calling the ``Enable()`` function.

This following is an example of creating two watchdog sources, one without and second one with the automatic feeding feature enabled:

.. code-block:: c++

    #include "watchdog/watchdog.h"

    void callback(Nrf::Watchdog::WatchdogSource *source)
    {
        if (source)
        {
            source->Feed();
        }
    }

    Nrf::Watchdog::WatchdogSource myWatchdog1;
    Nrf::Watchdog::WatchdogSource myWatchdog1(100, callback);

    if(Nrf::Watchdog::InstallSource(myWatchdog) && Nrf::Watchdog::InstallSource(myWatchdog1))
    {
        Nrf::Watchdog::Enable();
    }


.. note::

    The maximum number of simultaneously active watchdog sources is 8.
    Creating a ninth watchdog source will cause the ``InstallSource(WatchdogSource &source)`` function fail.

Feeding the Matter watchdog
***************************

Each Matter watchdog source must be fed separately in a specific context.
There are two options for feeding it:

Feeding automatically
  You can utilize the automatic feeding feature of the Matter watchdog to periodically feed the Matter watchdog source within a defined time.
  This approach can cause a higher power consumption, especially when the feedback interval is set to a low value.
  If you want to keep low power consumption, consider using the manual feeding approach instead.

  To configure and enable the automatic feeding feature, follow these steps:

  1. Include the ``watchdog/watchdog.h`` file.

     .. code-block:: c++

        #include "watchdog/watchdog.h"

  2. Declare and define the ``FeedingCallback`` implementation in your application code.
     This implementation must be tailored to your needs.

     For example, here is an implementation of ``FeedingCallback`` for feeding the Matter watchdog source within the Main thread:

     .. code-block:: c++

        void FeedFromApp(Nrf::Watchdog::WatchdogSource *source)
        {
	        if (source)
            {
		        Nrf::PostTask([source] { source->Feed(); });
	        }
        }

  3. Create a Matter watchdog source and provide the ``uint32_t feedingInterval``, and ``FeedingCallback callback`` optional arguments to the Matter watchdog source constructor.

     For example, here is a Matter watchdog source object configured to call the previously defined callback every 200 ms:

     .. code-block:: c++

        Nrf::Watchdog::WatchdogSource myWatchdog(200, FeedFromApp);

  4. Try to install the Matter watchdog source and check the result of the function:

     .. code-block:: c++

        if(!Nrf::Watchdog::InstallSource(myWatchdog))
        {
            LOG_ERR("Watchdog source cannot be installed.");
        }

  5. Enable the Global Watchdog module and check the function result:

     .. code-block:: c++

        if(!Nrf::Watchdog::Enable())
        {
            return false;
        }

Feeding manually
  Manual feeding involves calling the ``Feed()`` method of the specific source.
  This approach provides the best power consumption, because feeding only takes place if the CPU is not in sleep mode.

  To create, enable, and manually feed the Matter Watchdog source, complete the following steps:

  1. Create a Matter Watchdog source without any arguments.

     For example:

     .. code-block:: c++

        Nrf::Watchdog::WatchdogSource myWatchdog;

  2. Try to install the Matter watchdog source and check the result of the function:

     .. code-block:: c++

            if(!Nrf::Watchdog::InstallSource(myWatchdog))
            {
                LOG_ERR("Watchdog source cannot be installed.");
            }

  3. Enable the Global Watchdog module and check the function result:

    .. code-block:: c++

        if(!Nrf::Watchdog::Enable())
        {
            return false;
        }

  4. Call the ``Feed()`` method at the specific place in the code where you want to prevent code blocking:

     .. code-block:: c++

            myWatchdog.Feed();


.. note::

    If the ``InstallSource(WatchdogSource &source)`` function returns ``false``, it can mean that there is no available watchdog channel to assign.
    Ensure that you have at least one of the eight possible channels available.
    The function can also return ``false`` when the global watchdog module is not declared in the Devicetree specification file.
    Ensure that the module is properly declared.

Enabling and disabling the watchdog peripheral
**********************************************

The Global watchdog used in the |NCS| Matter samples is a single peripheral that operates independently of the CPU cores and includes multiple channels.
Although it is necessary to feed channels within their respective time windows separately, you cannot disable an individual channel without disabling the entire watchdog peripheral.
Instead, you can disable the entire watchdog peripheral, and if you wish to re-enable it, you must also restore all other watchdog sources.

To enable the Global watchdog module, use the ``Nrf::Watchdog::Enable()`` function and verify whether result of the function is ``true``:

.. code-block:: c++

    if(!Nrf::Watchdog::Enable())
    {
        return false;
    }

If the ``Nrf::Watchdog::Enable()`` function returns ``false``, it means that there is no watchdog sources installed, or there is a problem with starting the global watchdog timer.

From this point, all previously created Matter watchdog sources must be fed periodically to comply with the time window requirement.

To disable the Global watchdog module, use the ``Nrf::Watchdog::Disable()`` function:

.. code-block:: c++

    Nrf::Watchdog::Disable();

This method disables all previously enabled Matter watchdog sources, removes their channels, and stops the automatic feeding (if it was configured and enabled).

To disable a specific Matter watchdog source, delete the created object.

Limited lifetime watchdog source
********************************

A watchdog source can be created locally and removed when the objects is deleting.

For example, you can create a source in a specific function:

.. code-block:: c++

    void DoTask()
    {
        Nrf::Watchdog::WatchdogSource watchdogSource;

        if(!Nrf::Watchdog::InstallSource(myWatchdog))
        {
            LOG_ERR("Watchdog source cannot be installed.");
        }

        {
            while(condition)
            {
                /* Do some time-critical operations and break loop */
                watchdogSource.Feed();
            }
        }
    }

In the example function above, when the watchdog source object is created, a new watchdog is also created and it must be fed periodically.
After performing some time-critical operations and exiting the loop in the ``DoTask`` function, the watchdog source object is deleted, and its destructor frees the assigned channel.

.. note::

    The maximum number of simultaneously active watchdog sources is 8.
    Creating a ninth watchdog source will cause the ``InstallSource(WatchdogSource &source)`` function fail.

Default Matter watchdog implementation
**************************************

In the Matter common module, there is a default implementation of two watchdog sources that are automatically created for the release build version of a Matter sample.
One source is dedicated to monitoring the Main thread, and the other is dedicated to monitoring the Matter thread.
If at least one of the threads is blocked for a longer time than the value specified in the :option:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_TIMEOUT` Kconfig option, a reboot will occur.
The ``Nrf::Watchdog::Enable()``, and ``InstallSource(WatchdogSource &source)`` functions are called automatically.

To disable the default Matter watchdog implementation, set the :option:`CONFIG_NCS_SAMPLE_MATTER_WATCHDOG_DEFAULT` Kconfig option to ``n``.
