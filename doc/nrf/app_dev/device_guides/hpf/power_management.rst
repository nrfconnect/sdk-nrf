####################
HPF Power Management
####################

Power Management in HPF should be considered in two aspects:

* Host side Power Management
* HPF FW side Power Management

Host side Power Management
==========================

Host is responsible for managing power state of following resources:

* VPR with HPF FW running on it
* RAM

Optimal power state is achieved using `Device Runtime Power Management`_ and `System Power Off`_.

Device Runtime Power Management
-------------------------------

Each HPF device driver instance should call ``pm_device_runtime_get(hpf_dev)`` during its initialization
and ``pm_device_runtime_put(hpf_dev)`` when deinitialized.
This allows for the VPR driver to handle efficiently cases when multiple HPF peripherals are running on one VPR.

VPR coprocessor driver should implement transitions to ``PM_DEVICE_ACTION_SUSPEND`` and ``PM_DEVICE_ACTION_RESUME`` by
signalling to VPR about state change. Relevant handling will then be done on the HPF FW side.

Flow
----

Host side Power Management flow example is presented below:

  .. uml::

    @startuml
    participant Zephyr
    participant Application
    participant "mSPI driver"
    participant "VPR drivers"
    participant FLPR

    activate Zephyr
    Zephyr -> Zephyr : post kernel initialization
    activate Zephyr
    Zephyr -> "VPR drivers" : Initialize drivers
    activate "VPR drivers"
    rnote over "VPR drivers"
    Power on FLPR RAM
    endrnote
    rnote over "VPR drivers"
    If non-XIP:
    Copy code to RAM
    endrnote
    rnote over "VPR drivers"
    Start VPR
    endrnote

    "VPR drivers" -> "VPR drivers" : Initialize IPC
    activate "VPR drivers"
    "VPR drivers" -> FLPR
    activate FLPR
    rnote over FLPR
    Boot
    endrnote
    rnote over FLPR
    Initialize IPC
    endrnote
    return
    deactivate "VPR drivers"

    "VPR drivers" -> "VPR drivers" : Initialize FLPR
    activate "VPR drivers"
    "VPR drivers" -> FLPR : FLPR_INIT
    activate FLPR
    rnote over FLPR
    Initialize HW, peripherals, etc
    endrnote
    return FLPR_INITD
    deactivate "VPR drivers"
    return
    deactivate "VPR drivers"


    deactivate Zephyr
    Zephyr -> Application : Start application
    activate Application
    Application -> "mSPI driver" : mspi_config(...)
    activate "mSPI driver"
    "mSPI driver" -> "mSPI driver" : mSPI initialization
    activate "mSPI driver"
    "mSPI driver" -> "VPR drivers" : pm_device_runtime_get(&flpr)
    activate "VPR drivers"
    "VPR drivers" -> FLPR : FLPR_RESUME
    activate FLPR
    rnote over FLPR
    Enter RESUME state
    endrnote
    return FLPR_RESUMED
    return

    "mSPI driver" -> FLPR : FLPR_MSPI_CONFIGURE
    activate FLPR
    rnote over FLPR
    Configure emulated MSPI
    endrnote
    return FLPR_MSPI_CONFIGURED  
    deactivate "mSPI driver"
    return
    ...


    Application -> "mSPI driver" : mspi_transcieve(...)
    activate "mSPI driver"
    "mSPI driver" -> FLPR : FLPR_MSPI_XFER
    activate FLPR
    rnote over FLPR
    Execute MSPI transfer
    endrnote
    return FLPR_MSPI_XFERD
    return
    ...


    Application -> "mSPI driver" : mspi_config(...)
    activate "mSPI driver"
    "mSPI driver" -> "mSPI driver" : mSPI initialization
    activate "mSPI driver"
    "mSPI driver" -> FLPR : FLPR_MSPI_DECONFIGURE
    activate FLPR
    rnote over FLPR
    Deconfigure emulated MSPI
    endrnote
    return FLPR_MSPI_DECONFIGURED

    "mSPI driver" -> "VPR drivers" : pm_device_runtime_put(&flpr)
    activate "VPR drivers"
    "VPR drivers" -> FLPR : FLPR_SUSPEND
    activate FLPR
    rnote over FLPR
    Enter SUSPENDED state
    endrnote
    return FLPR_SUSPENDED
    return
    deactivate "mSPI driver"
    return
    return


    Zephyr -> Zephyr : power off procedure
    activate Zephyr
    "Zephyr" -> "VPR drivers" : power off
    activate "VPR drivers"
    "VPR drivers" -> "VPR drivers" : FLPR shutdown
    activate "VPR drivers"
    "VPR drivers" -> FLPR : FLPR_SHUTDOWN_PREPARE
    activate FLPR
    rnote over FLPR
    Deinitialize HW peripherals, etc
    endrnote
    return FLPR_SHUTDOWN_PREPARED
    deactivate "VPR drivers"
    rnote over "VPR drivers"
    Power off VPR
    endrnote
    rnote over "VPR drivers"
    Power off FLPR RAM
    endrnote
    return
    deactivate Zephyr
    @enduml

HPF FW side Power Management
============================

HPF FW is responsible for managing power state of the following resources:

* VPR CPU power state
* HW peripherals used by HPF FW

VPR CPU power state
-------------------

Considering that HPF firmware will not use Zephyr libraries for `System Power Management`_ (since they rely on multithreading), a separate approach has to be implemented for this use case.

HPF firmware should configure sleep mode that will be triggered (using both ``SLEEPSTATE`` and ``STACKONSLEEP`` fields in ``NORDIC.VPRNORDICSLEEPCTRL`` register - see ``Sleep mode operation`` section in VPR peripheral description), when:

* VPR is started (sleep mode to set comes from a Kconfig option).
* `SUSPEND` or `RESUME` signal is received from Host.

In the main loop, HPF FW should call ``k_cpu_idle`` to enter sleep mode.
``k_cpu_idle()`` can only be called in the main loop. Calling it from other context is prohibited, as it could lead to VPR entering an incorrect power state.

Sleep modes to use should be application-specific, taking into consideration:

* Latency requirements of the emulated protocol.
* Minimum time in sleep mode that brings power benefits.
* HW limitations (for example limits in wakeup from hibernation).

HW peripherals used by HPF FW
-----------------------------

HPF may require HW peripherals to be used by HPF FW.

When VPR is started, it should initialize required peripherals. When Host signals a request to shutdown, peripherals should be uninitialized and powered off before signalling completion of preparation for shutdown.

HPF FW must ensure optimal power state of HW peripherals, therefore it should use nrfx drivers that handle HW peripherals' power management.


.. _`Device Runtime Power Management`: https://docs.zephyrproject.org/latest/services/pm/device_runtime.html#device-runtime-power-management
.. _`System Power Off`: https://docs.zephyrproject.org/latest/doxygen/html/group__sys__poweroff.html
.. _`System Power Management`: https://docs.zephyrproject.org/latest/services/pm/system.html#system-power-management
