.. _hpf_power_management:

Power management
################

The following page outlines the approaches and mechanisms employed to ensure efficient power management in High-Performance Framework (HPF) systems, focusing on resource allocation, system states, and interaction between different components.

Power Management in HPF should be considered from the Host side and the HPF firmware side.

Host side
*********

The Host is responsible for managing power state of the following resources:

* VPR core with HPF firmware running on it
* RAM

To achieve optimal power state, you must use the `Device Runtime Power Management`_ and `System Power Off`_.

See an example flow of the Host power management:

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

Device runtime power management
===============================

Each HPF device driver instance should call ``pm_device_runtime_get(hpf_dev)`` during its initialization
and ``pm_device_runtime_put(hpf_dev)`` upon deinitialization.
This ensure efficient handling of cases where multiple HPF peripherals are running on a single VPR core.

VPR coprocessor driver is responsible for managing transitions to ``PM_DEVICE_ACTION_SUSPEND`` and ``PM_DEVICE_ACTION_RESUME`` by signaling these state changes to the VPR.
The HPF firmware on the other side handles the corresponding actions.

HPF firmware
************

HPF FW is responsible for managing power state of the following resources:

* VPR CPU power state
* Hardware peripherals used by the HPF firmware

VPR CPU power state
===================

HPF firmware does not use Zephyr libraries for `System Power Management`_ (since they rely on multithreading), you must implement a separate approach.

HPF firmware should configure a sleep mode that is activated using both the ``SLEEPSTATE`` and ``STACKONSLEEP`` fields in ``NORDIC.VPRNORDICSLEEPCTRL`` register.
For details, see the *Sleep mode operation* section in the VPR peripheral description.
This configuration should occur in the following cases:

* When the VPR starts (the sleep mode setting is derived from a Kconfig option).
* When the Host receives the ``SUSPEND`` or ``RESUME`` signal.

In the main loop, HPF firmware should call ``k_cpu_idle`` to enter sleep mode.
Ensure that ``k_cpu_idle()`` is only called within the main loop to prevent the VPR from entering an incorrect power state.

Sleep modes should be application-specific.
Consider the following:

* Latency requirements of the emulated protocol.
* Minimum time in sleep mode that brings power savings.
* Hardware limitations (for example, constraints on waking up from hibernation).

Hardware peripherals
====================

You might have to use specific hardware peripherals with the HPF firmware.

When VPR starts, it must initialize the required peripherals.
When the Host sends a request for shutdown, these peripherals should be deinitialized and powered down before signaling that preiparations for shutdown are complete.

To ensure the optimal power state of hardware peripherals, the HPF firmware should employ nrfx drivers, which are designed to manage the power states of hardware peripherals effectively.
