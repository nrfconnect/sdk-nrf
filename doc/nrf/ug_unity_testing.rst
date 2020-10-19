.. _ug_unity_testing:

Testing with Unity and CMock
############################

.. contents::
   :local:
   :depth: 2

The |NCS| provides support for writing unit tests using Unity and CMock.
`Unity`_ is a C unit test framework.
`CMock`_ is a framework for generating mocks based on a header API.
Support for CMock is integrated into Unity.

CMake automatically generates the required test runner file and the mock files.
The test can be executed on the :ref:`zephyr:native_posix` board.

Setting up a unit test
**********************

An example for a unit test can be found in :file:`tests/unity/example_test`.
The file `tests/unity/example_test/CMakeLists.txt`_ shows how to set up the generation of the test runner file and mocks.

To run the unit test, enable the Unity module (:option:`CONFIG_UNITY`) and the module under test.
The module under test and all dependencies are enabled and compiled into the binary, even the mocked modules.

The linker replaces all calls to the mocked API with a mock implementation using the wrapping feature.
For example, :code:`-Wl,--wrap=foo` replaces every call to :code:`foo()` with a call to :code:`__wrap_foo()`.
Because of that, all mock functions are prefixed with :code:`__wrap` (for example, :code:`__wrap_foo_Expect()` instead of :code:`foo_Expect()`).

To specify the APIs that should be mocked in a given test, edit :file:`CMakeLists.txt` to call :code:`cmock_handle` with the header file and, optionally, the relative path to the header as arguments.
The relative path is needed if a file is included as, for example, :code:`#include <bluetooth/gatt.h>`.
In this case, the relative path is ``bluetooth``.

Add a line similar to the following in :file:`CMakeLists.txt` to generate the mocks::

     cmock_handle(${ZEPHYR_BASE}/include/bluetooth/gatt.h bluetooth)

The test runner file must be generated from the file that contains the tests.
Unity treats all functions that are prefixed with ``test_`` as tests.

Add a line similar to the following in :file:`CMakeLists.txt` to generate the test runner file::

     test_runner_generate(src/example_test.c)

Running the test
****************

Enter the following command to generate the test project (including the test runner file and the mocks) and run it::

     west build -b native_posix -t run
