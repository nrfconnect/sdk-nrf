# Vendor commands have been implemented as an extension to Openthread CLI commands.

## Vendor commands description:

### ot help

Shows all available Openthread CLI commands.

### ot vendor:diag:attenuation \[\(0-15\)\]

Get/set attenuation value.
> `ot vendor:diag:fem:enable` needs to be enabled first.

## ot vendor:diag:fem:enable

Enable/disable controlling attenuation and radio tx power by cmds: `ot vendor:diag:attenuation` and `ot vendor:diag:radiotxpower`.

## ot vendor:diag:radiotxpower

Get/set radio tx power.
It's limited to radio supported values.
> `ot vendor:diag:fem:enable` needs to be enabled first.

## ot vendor:fem \[\<disable|vc0|vc1|bypass|auto\>\]

Get/set FEM state.

## ot vendor:fem:bypass \[\<on|off\>\]

Get/set bypass mode

> This is an obsolate cmd, use `ot vendor:fem bypass` instead.

## ot vendor:power:limit:active:id \[id\]

Get/set power limit table id.

## ot vendor:power:limit:active:id:invalidate

Invalidates the power limit id, effectively disabling the tx power limit.

## ot vendor:power:limit:table

Get power limit table for active radio tx path.

## ot vendor:power:limit:table:limit \<channel\>

Get the power limit for a given radio channel.

## ot vendor:power:limit:table:version

Get the power limit table version.
The power limit table version for FEM PSEMI equals 1.

## ot vendor:power:mapping:table

Get the power map table for the active radio tx path.

## ot vendor:power:mapping:table:clear

Clear the power map table stored in non-volatile memory.

## ot vendor:power:mapping:table:flash:data \[data\]

Get/set power map table from/to non-volatile memory.

```
ot vendor:power:mapping:table:flash:data 0516000200000302ffec00000004ffffecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404ecececececececececececececececec0000000000000000000000000000000004040404040404040404040404040404fff0fffcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fcfcfcfcfcfcfcfcfcfcfcfcfcfcfcfcef779a5b
Done
```

> In case of setting the new power map table, the partition in non-volatile memory must be previously cleared.

## ot vendor:power:mapping:table:flash:data:valid

Check if power map partition stores the valid power map table.

## ot vendor:power:mapping:table:test \[channel requested_power\]

Get RadioTxPower, FemGain and OutputPower for given channel and requested_power based on active power map table.
Example:
```
ot vendor:power:mapping:table:test 11 6
Done

ot vendor:power:mapping:table:test
RadioTxPower=6
FemGain=20
OutputPower=6
Done
```

> The calculation does not concern the power limit.

## ot vendor:power:mapping:table:version

Get power map table version.
Power map table version for FEM PSEMI equals 2.

## ot vendor:temp

Get temperature value is in Celsius scale, multiplied by 100.
For example, the actual temperature of 25.75\[C\] will be returned as a 2575 signed integer.
Measurement accuracy is 0.25\[C\].

## ot vendor:clkout:lfclk

Enables/disables GRTC low-frequency clock output on the corresponding clock pin.

```
ot vendor:clkout:lfclk start
Done

ot vendor:clkout:lfclk stop
Done
```

## ot vendor:devmem

Read device memory at the specified address.
Optionally, you can specify the access width (8, 16, or 32 bits).

```
ot vendor:devmem 0x00FFC31C
Using data width 32
Read value 0x54b15
Done
```