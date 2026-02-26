.. _bluetooth-dev:

Application Development
#######################

Bluetooth applications are developed using the common infrastructure and
approach that is described in the :ref:`application` section of the
documentation.

Additional information that is only relevant to Bluetooth applications can be
found on this page.

.. contents::
    :local:
    :depth: 2

Thread safety
*************

Calling into the Bluetooth API is intended to be thread safe, unless otherwise
noted in the documentation of the API function. The effort to ensure that this
is the case for all API calls is an ongoing one. Bug reports and Pull Requests
that move the subsystem in the direction of such goal are welcome.
