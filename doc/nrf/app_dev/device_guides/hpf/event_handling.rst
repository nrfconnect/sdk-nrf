##############
Event handling
##############

Requirements
============

#. Modularity

   - Combining functionalities of multiple HPF peripherals should be possible.

#. Timing accuracy

   - Certain parts of code need to be handled with very accurate timings.

#. Verification

   - It should be possible to verify timing accuracy without a need for re-running a whole suite of HW characteristic tests.

Implementation
==============

Parts of code that need to be handled with very accurate timings will be referred to as Hard-Real Time (HRT) functions.

HRT functions will reside separately from application code. (Modularity, Verification)

During a build, HRT functions will be compiled to assembly code. This code will be checked against previous version of the assembly code that was verified with HW characteristic tests.  If the difference is none, or deemed small enough, new code is considered timing accurate as well. (Verification)

HRT functions will be written using HAL, without calls to external functions. Macros (including external ones) that do not expand to external functions are allowed. If external macro is changed and impacts the assembly code, it should be redefined locally. (Timing accuracy)

HRT functions will be implemented as interrupt handlers, to ensure they are executed in correct order when triggered from multiple sources. (Timing accuracy)

Interaction between HRT and non-HRT code
========================================

Communication
-------------

HRT code is implemented in ISR and cannot use function arguments to pass values.
Therefore, following methods to communicate between non-HRT and HRT contexts can be used:

   - VEVIF interrupt ID: Separate HRT functions can be assigned to certain VEVIF IRQ.
   - Shared memory: Data can be stored in pre-defined memory addresses.

Sequence flow
-------------
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
