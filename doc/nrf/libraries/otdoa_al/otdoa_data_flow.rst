.. _otdoa data flow:

hellaPHY OTDOA Data Flow
========================

As described in :ref:`otdoa_overview`, the hellaPHY OTDOA firmware subsystem consists of a binary library
(delivered in binary object code format), and the adaptation layer (delivered as source code as part
of the nRF Connect SDK). This section describes the flow of data through these components, including
download of the uBSA, estimate of the time of arrival for signals from multiple cells, and formulation
of the position estimate.

When the hellaPHY OTDOA library is requested to make a position estimate, it retrieves the current uBSA
information from the file system storage, and uses it to generate a list of cells for measurement.
This list is known as  the "assistance data," and includes cells in the local geographic area.

The library then uses the nrfxlib RS Capture API to receive Positioning Reference Signals
(PRS) transmitted by cells in the assistance data. The RS capture API typically provides a block
of samples for processing every 160 ms. These samples are written into application processor buffers
(a Zephyr memory slab) and forwarded to the OTDOA thread for processing. The OTDOA
thread wakes up, processes the samples through the OTDOA library, and then frees the
sample buffer, making it available for the next batch of samples.

Once it has detected a sufficient number of cells, the OTDOA library estimates the time of
arrival for the PRS signal from each detected cell. It then selects the best receive cell
as its reference cell, and calculates the time-difference of arrival for each cell by subtracting
the reference cell time of arrival from each other cell's time of arrival. The library
then uses the time-difference of arrival values, and each cell's known location, to calculate
an estimate of the UE's position.

The hellaPHY OTDOA library also includes an Enhanced Cell ID (ECID) algorithm used as a fallback to
estimate the UE position when it cannot detect the PRS signals from a sufficent number
of base stations. The library also includes algorithms to estimate the accuracy of
its OTDOA and ECID position estimates.

The hellaPHY OTDOA binary library executes almost completely on a single Zephyr thread. An exception to this is
the OTDOA API (see :doc:`phywi_api`), where API function calls are converted to messages
that are sent to the thread for processing. Another exception is the callback used by
the Nordic RS Capture API, which writes samples into its memory slab buffer and then sends a
message to the thread to indicate the samples are available for processing. Finally, the hellaPHY OTDOA
Adaptation layer includes a workqueue thread that allows it to interact with a network server to download
the uBSA.

Real-Time Constraints
---------------------

The hellaPHY OTDOA binary library is designed to process the PRS from the modem as they are received. Typically
a new block of samples is received every 160 ms. The library processing for a block typically
requires only 15% to 20% of this 160 ms. interval. This leaves plenty of processing time for other
application functions, even while the hellaPHY OTDOA positioning system is active.

If the hellaPHY OTDOA library is not able to keep up with the processing of the PRS
signals, some blocks of samples will be dropped as they are received from the RS Capture
API. This loss of data may result in a slight decrease in the performance of the
OTDOA algorithms, but otherwise it is not harmful.

The buffering of the PRS samples is done using a memory slab in the hellaPHY OTDOA adaptation layer.
Typically, this buffer is sized to hold a single block of samples. This requires that
the hellaPHY OTDOA library complete processing of one block of samples before the next block
of samples is received from the nrfxlib RS Capture API. If the application processor
loading is particularly high, the size of the memory slab can be increased to allow
additional time for the OTDOA library to complete its processing. However, this is
not typically required.
