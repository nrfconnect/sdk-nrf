Profiler Test
-------------

The test suite consists of three performance tests.
The tests do not check whether data is transmitted.
To examine it, one has to collect data transmitted to host using a Profiler backend's host tool and check manually whether the data is correct.

The expected output looks as follows:

1. 100 events named "no data event" with no data.
2. 100 events named "data event" with one value named "value1" of type "u32".
   The value starts from 0 and is incremented by one with every event.
3. 100 events named "big event" with six values and one string. The values and string should be:
	a) "value1"
		-type: "u32"
		-value: starts from 0 and is incremented by one with every event.
	b) "value2"
		-type: "s32"
		-value: starts from -50 and is incremented by one with every event.
	c) "value3"
		-type: "u16"
		-value: starts from 0 and is incremented by one with every event.
	d) "value4"
		-type: "s16"
		-value: starts from -50 and is incremented by one with every event.
	e) "value5"
		-type: "u8"
		-value: starts from 0 and is incremented by one with every event.
	f) "value6"
		-type: "s8"
		-value: starts from -50 and is incremented by one with every event.
	g) "string"
		-type: "s"
		-value: 'example string'
