.. _stack_size_analyzer:

Stack size analyzer
###################

The stack size analyzer module enables all the Zephyr options required to track
the stack usage.
The analysis is performed on demand when the application calls
:cpp:func:`stack_size_analyzer_run` or :cpp:func:`stack_size_analyzer_print`.

Configuration
*************
Configure this module using the following options.

* ``STACK_SIZE_ANALYZER``: enable the module.
* ``STACK_SIZE_ANALYZER_USE_PRINTK``: use printk for stack statistics.
* ``STACK_SIZE_ANALYZER_USE_LOG``: use the logger for stack statistics.
* ``STACK_SIZE_ANALYZER_AUTO``: run the stack analyzer automatically.
  You do not need to add any code to the application when using this option.
* ``STACK_SIZE_ANALYZER_AUTO_PERIOD``: the time for which the module sleeps
  between consecutive printing of stack size analysis in automatic mode.
* ``STACK_SIZE_ANALYZER_AUTO_STACK_SIZE``: the stack size for stack size analyzer
  automatic thread.
* ``THREAD_NAME``: enable this option in the kernel to print the name of the thread
  instead of its ID.

API documentation
*****************

| Header file: :file:`include/debug/stack_size_analyzer.h`
| Source files: :file:`subsys/debug/stack_size_analyzer/`

.. doxygengroup:: stack_size_analyzer
   :project: nrf
   :members:
