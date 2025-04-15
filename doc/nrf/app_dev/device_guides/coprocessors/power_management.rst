.. _hpf_power_management:

Power management
################

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

The following page outlines the approaches and mechanisms employed to ensure efficient power management in High-Performance Framework (HPF) systems, focusing on resource allocation, system states, and interactions between different components.

.. note::
   This page outlines best practices for architecture design.
   You might need to implement some components, as they are not yet available.

Power management strategies
***************************

Power Management in HPF should be considered from two perspectives: the Host controller and the HPF firmware.

Host controller
===============

The Host is responsible for managing power state of the following resources:

* FLPR core with HPF firmware running on it
* RAM

To achieve optimal power state, you must use the `Device Runtime Power Management`_ and `System Power Off`_.

See an example flow of the Host power management:

.. uml::

  @startuml
  skinparam sequence {
  DividerBackgroundColor #8DBEFF
  DividerBorderColor #8DBEFF
  LifeLineBackgroundColor #13B6FF
  LifeLineBorderColor #13B6FF
  ParticipantBackgroundColor #13B6FF
  ParticipantBorderColor #13B6FF
  BoxBackgroundColor #C1E8FF
  BoxBorderColor #C1E8FF
  GroupBackgroundColor #8DBEFF
  GropuBorderColor #8DBEFF
  }

  skinparam note {
  BackgroundColor #ABCFFF
  BorderColor #2149C2
  }

  participant Zephyr
  participant Application
  participant "HPF driver"
  participant "VPR driver"
  participant FLPR

  activate Zephyr
  Zephyr -> Zephyr : post kernel initialization
  activate Zephyr
  Zephyr -> "VPR driver" : Initialize drivers
  activate "VPR driver"
  rnote over "VPR driver"
  Power on FLPR RAM
  endrnote
  rnote over "VPR driver"
  If non-XIP:
  Copy code to RAM
  endrnote
  rnote over "VPR driver"
  Start FLPR
  endrnote

  "VPR driver" -> "VPR driver" : Initialize IPC
  activate "VPR driver"
  "VPR driver" -> FLPR
  activate FLPR
  rnote over FLPR
  Boot
  endrnote
  rnote over FLPR
  Initialize IPC
  endrnote
  return
  deactivate "VPR driver"

  "VPR driver" -> "VPR driver" : Initialize FLPR
  activate "VPR driver"
  "VPR driver" -> FLPR : FLPR_INIT
  activate FLPR
  rnote over FLPR
  Initialize HW, peripherals, etc
  endrnote
  return FLPR_INITD
  deactivate "VPR driver"
  return
  deactivate "VPR driver"


  deactivate Zephyr
  Zephyr -> Application : Start application
  activate Application
  Application -> "HPF driver" : hpf_driver_config(...)
  activate "HPF driver"
  "HPF driver" -> "HPF driver" : HPF driver initialization
  activate "HPF driver"
  "HPF driver" -> "VPR driver" : pm_device_runtime_get(&flpr)
  activate "VPR driver"
  "VPR driver" -> FLPR : FLPR_RESUME
  activate FLPR
  rnote over FLPR
  Enter RESUME state
  endrnote
  return FLPR_RESUMED
  return

  "HPF driver" -> FLPR : HPF_APP_CONFIGURE
  activate FLPR
  rnote over FLPR
  Configure HPF application
  endrnote
  return HPF_APP_CONFIGURED
  deactivate "HPF driver"
  return
  ...


  Application -> "HPF driver" : hpf_driver_transcieve(...)
  activate "HPF driver"
  "HPF driver" -> FLPR : HPF_APP_XFER
  activate FLPR
  rnote over FLPR
  Execute transfer
  endrnote
  return HPF_APP_XFERD
  return
  ...


  Application -> "HPF driver" : hpf_driver_config(...)
  activate "HPF driver"
  "HPF driver" -> "HPF driver" : HPF driver deinitialization
  activate "HPF driver"
  "HPF driver" -> FLPR : HPF_APP_DECONFIGURE
  activate FLPR
  rnote over FLPR
  Deconfigure HPF application
  endrnote
  return HPF_APP_DECONFIGURED

  "HPF driver" -> "VPR driver" : pm_device_runtime_put(&flpr)
  activate "VPR driver"
  "VPR driver" -> FLPR : FLPR_SUSPEND
  activate FLPR
  rnote over FLPR
  Enter SUSPENDED state
  endrnote
  return FLPR_SUSPENDED
  return
  deactivate "HPF driver"
  return
  return


  Zephyr -> Zephyr : power off procedure
  activate Zephyr
  "Zephyr" -> "VPR driver" : power off
  activate "VPR driver"
  "VPR driver" -> "VPR driver" : FLPR shutdown
  activate "VPR driver"
  "VPR driver" -> FLPR : FLPR_SHUTDOWN_PREPARE
  activate FLPR
  rnote over FLPR
  Deinitialize HW peripherals, etc
  endrnote
  return FLPR_SHUTDOWN_PREPARED
  deactivate "VPR driver"
  rnote over "VPR driver"
  Power off FLPR
  endrnote
  rnote over "VPR driver"
  Power off FLPR RAM
  endrnote
  return
  deactivate Zephyr
  @enduml

.. note::
	VPR driver is not yet implemented, it should be implemented when writing HPF software.
	It shall run on application core.
	Its purpose is to control basic FLPR functionalities like initialization of inter processor communication and power mode management.

HPF firmware
============

HPF firmware is responsible for managing power states of the following resources:

* FLPR CPU power state
* Hardware peripherals used by the HPF firmware

FLPR CPU power state management
-------------------------------

Your HPF firmware should not use the Zephyr libraries for `System Power Management`_, as the libraries rely on multithreading.
Instead, you must implement a distinct approach.

Your HPF firmware should configure a sleep mode that is activated using both the ``SLEEPSTATE`` and ``STACKONSLEEP`` fields in the ``NORDIC.VPRNORDICSLEEPCTRL`` register.
For details, see the *Sleep mode operation* section in the VPR peripheral description.

This configuration must occur in the following cases:

* When the FLPR starts (the sleep mode setting is derived from a Kconfig option).
* When the Host receives the ``SUSPEND`` or ``RESUME`` signal.

In the main loop, the HPF firmware should call ``k_cpu_idle`` to enter sleep mode.
Ensure that ``k_cpu_idle()`` is only called within the main loop to prevent the FLPR from entering an incorrect power state.

Sleep modes should be application-specific.
Consider the following:

* Latency requirements of the emulated protocol
* Minimum time in sleep mode that brings power savings
* Hardware limitations (for example, constraints on waking up from hibernation)

Hardware peripherals
--------------------

You might have to use specific hardware peripherals with the HPF firmware.

When FLPR starts, it must initialize the required peripherals.
When the Host sends a request for shutdown, these peripherals should be deinitialized and powered down before signaling that preparations for shutdown are complete.

To ensure the optimal power state of hardware peripherals, the HPF firmware should employ nrfx drivers, which are designed to manage the power states of hardware peripherals effectively.
