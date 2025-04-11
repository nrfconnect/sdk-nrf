.. _hpf_event_handling:

Event handling
##############

This page section outlines the critical requirements and implementation strategies for managing events within the system, ensuring that functionalities are both modular and precise in execution.

Requirements
************

The system must support the integration of multiple High-Performance Framework (HPF) peripherals, ensuring the following:

* Modularity, allowing to combine functionalities of multiple HPF peripherals.
* Timing accuracy, focusing on handling certain parts of the code with very accurate timings.
* Verification, ensuring it is possible verify timing accuracy without the need to rerun an entire suite of hardware characteristic tests.

Implementation
**************

To ensure precise timing, specific sections of the code are identified and managed as Hard-Real Time (HRT) functions.
These functions are kept separate from the main application code.

During the build process, HRT functions are compiled into assembly code.
This code is then compared to a previously verified version to ensure that there are no significant deviations.
If the differences are minimal or non-existent, the new code will be deemed accurate in terms of timing.

HRT functions are developed using the Hardware Abstraction Layer (HAL) and avoid any calls to external functions.
While you can use macros that do not lead to external function calls, you must redefine locally any external macro that influences the assembly code to maintain the required timing accuracy.

Furthermore, HRT functions are implemented as interrupt handlers.
This implementation ensures that these functions are executed in the correct sequence when triggered by multiple sources, maintaining the system's timing accuracy.

Interaction between HRT and non-HRT code
****************************************

Communication between HRT and non-HRT code is critical, especially since HRT functions operate within an Interrupt Service Routine (ISR) and cannot use function arguments for passing values.
To facilitate this communication, you can use the following methods: specific VEVIF IRQs can be assigned to different HRT functions, and data can be stored in predefined memory addresses.

* VEVIF interrupt ID - You can assign separate HRT functions to certain VEVIF IRQs.
* Shared memory - You can store data in the pre-defined memory addresses.

The sequence flow is designed to handle initialization, transaction requests, and error handling efficiently.
During initialization, the system boots up and prepares for operation by entering a wait state.
For transaction requests, the system handles transmission and reception requests through IPC mechanisms, utilizing shared memory or IRQ numbers for communication between different parts of the system.

In the event of an error, such as a memory fault, the system triggers an error handler which notifies the host of the error and then enters an infinite loop to halt further operations.
This sequence ensures that the system maintains robust operation and handles errors effectively to prevent system failures:

//TODO: check uml
   .. uml::


     @startuml
      participant Thread as t
      participant "IPC IRQ \n(low priority)" as i
      participant "HRT IRQ \n(high priority)" as h
      participant "Error handler \n(top priority)" as e

      == Initialization ==

      [-> t : Start VPR
      activate t
      t -> t : Boot
      t -> t : Initialization
      t -> t : WFI
      deactivate t
      ...

      == TX-RX-TX requests ==

      [-> i : TX request from host
      activate i
      i -> i : IPC RX
      note right i: Shared mem and/or IRQ num
      i -> h : Dispatch
      deactivate i

      activate h
      h -> h : HRT TX execution
      [-> i : RX request from host
      h -> h : Mark TX as done
      return

      activate i
      i -> i : IPC RX
      opt
      i -> i : Notify host:\n"TX queue empty"
      end
      i -> h : Dispatch
      deactivate i

      activate h
      h -> h : HRT RX execution
      [-> i : TX request from host

      h -> h: Mark Rx as done
      return
      activate i

      i -> i : IPC TX
      i -> i : IPC RX
      i -> h : Dispatch
      deactivate i

      activate h
      h -> h : HRT TX execution
      h -> h : Mark TX as done
      return

      i --> t

      activate t
      t -> t : WFI
      deactivate t
      ...

      == Error handling ==
      [-> h : ...
      activate h
      h -> h : HRT TX execution
      e <-] : Memory fault
      deactivate h
      activate e
      e -> e : Notify host:\n"HPF error"
      e -> e : Infinite loop

     @enduml
