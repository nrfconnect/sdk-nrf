Nordic nrf_profiler is host application used to receive data through Nordic
nrf_profiler protocol and process it.

Usage:

python3 data_collector.py
Collects events from device and saves it to files.

python3 real_time_plot.py
Plots in real time events received from device. Then data is saved to files.

python3 plot_from_files.py
Plots events from files. In addition, after closing plot, calculated stats are
saved to log.csv file.

Using GUI while plotting:

- Start/Stop button below plot - pause or resume real time moving plot
- Scroll - zoom in/out (zooming to cursor location when paused or to right
	       	edge of the plot when plotting in real time)
- Click scroll/middle mouse button - mark event submit or processing to track
		the event.

Additionally during pause or when plotting data from files:
 - left/right mouse button - place horizontal line at cursor's location.
		When two lines are present application measures time between
		lines and writes it on plot.
 - left mouse button (click and drag) - pan the plot


Data structures used to handle events (events.py):
1. Event - single occurrence of event
	type_id - id of event type
	timestamp - timestamp of event (in seconds)
	data - data fields of event

2. EventType - type of event
	name - name of event type
	data_types - data types of event type datafields
	data_descriptions - descriptions of event type datafields

3. EventsData - structure combines event occurrences and event data types
received through RTT
	events - event occurrences - list of Event objects
	registered_events_types - dictionary of EventType objects
				  (key is event type id)
