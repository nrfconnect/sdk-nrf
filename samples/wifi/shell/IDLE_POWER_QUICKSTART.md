# Wi-Fi Shell Idle Power - Quick Start

## Build with Idle Power Optimization

### Minimal Configuration (for development)
```bash
west build -b nrf7120dk/nrf7120/cpuapp samples/wifi/shell -- \
  -DCONF_FILE="prj.conf;configs/idle_power_base.conf;configs/wifi_idle_optimization.conf"
```

### Full Optimization (for measurements)
```bash
west build -b nrf7120dk/nrf7120/cpuapp samples/wifi/shell -- \
  -DCONF_FILE="prj.conf;configs/idle_power_base.conf;configs/ram_power_down.conf;configs/wifi_idle_optimization.conf;configs/device_idle.conf" \
  -DDTC_OVERLAY_FILE="boards/nrf7120dk_nrf7120_cpuapp_idle_power.overlay"
```

### With Diagnostics (for debug/analysis)
```bash
west build -b nrf7120dk/nrf7120/cpuapp samples/wifi/shell -- \
  -DCONF_FILE="prj.conf;configs/idle_power_base.conf;configs/ram_power_down.conf;configs/wifi_idle_optimization.conf;configs/device_idle.conf;configs/diagnostics.conf" \
  -DDTC_OVERLAY_FILE="boards/nrf7120dk_nrf7120_cpuapp_idle_power.overlay"
```

### Quiet Mode (for power measurements without debug output)
```bash
west build -b nrf7120dk/nrf7120/cpuapp samples/wifi/shell -- \
  -DCONF_FILE="prj.conf;configs/quiet.conf;configs/idle_power_base.conf;configs/ram_power_down.conf;configs/wifi_idle_optimization.conf;configs/device_idle.conf" \
  -DDTC_OVERLAY_FILE="boards/nrf7120dk_nrf7120_cpuapp_idle_power.overlay"
```

## What Each Config Does

| Config | Purpose | When to Use |
|--------|---------|------------|
| `idle_power_base.conf` | Core PM + Tickless kernel | Always (foundation) |
| `ram_power_down.conf` | Power down unused RAM | When RAM can be reduced to 256 KiB |
| `wifi_idle_optimization.conf` | Wi-Fi radio PM | Always with Wi-Fi |
| `device_idle.conf` | Peripheral power domains | When measuring true idle |
| `diagnostics.conf` | Power state monitoring | During debug/measurement |
| `quiet.conf` | Remove UART/console | For clean power measurements |

## Shell Commands

```bash
# Check Wi-Fi status
wifi status

# Scan for networks
wifi scan

# Connect to network
wifi connect <ssid> <password>

# Disconnect from network
wifi disconnect

# Enable Wi-Fi power save (reduces power in connected mode)
wifi power_save on

# Check power save settings
wifi power_save status

# See all available commands
help
```

## Typical Idle Current

- **With optimizations enabled**: ~3-5 µA (system ON, Wi-Fi active)
- **Without optimizations**: ~50-100 µA
- **Power saving**: 10-20x reduction in idle consumption

## GPIO Signals for Measurement

**P0.00** - Idle Phase Marker
- HIGH = CPU running
- LOW = CPU in WFI (idle)

**P0.10** - Power Domain Signal (when diagnostics enabled)
- Reflects selected power domain status
- Configure via `CONFIG_NRF7120_PDSELECT_SIGNAL` (1-8, 11)

## Troubleshooting

**Wi-Fi doesn't work:**
- Disable `ram_power_down.conf` (increase RAM: `CONFIG_NRF7120_RAM_384K_ONLY=y`)

**Idle current not low:**
- Check LFXO is enabled
- Verify `CONFIG_NRF7120_FORCE_LOWPWR=y` is set
- Disable UART output (use `quiet.conf`)

**Crashes/resets:**
- Increase retained RAM size
- Disable diagnostics config
- Check stack sizes are adequate

## Power Measurement Setup

1. Connect PPK2 or similar current probe
2. Connect logic analyzer to P0.00 (idle marker)
3. Flash with appropriate config from above
4. Device reaches idle in ~1-2 seconds
5. Observe minimum current on power meter
6. Issue Wi-Fi commands from shell (`wifi scan`, etc.)

## Configuration Tips

- Start with `idle_power_base.conf` + `wifi_idle_optimization.conf`
- Add `ram_power_down.conf` only if Wi-Fi still works
- Use `diagnostics.conf` to debug power state issues
- Use `quiet.conf` for production measurements
- Keep `UART` enabled during development

## See Also

- Full documentation: `IDLE_POWER.rst`
- Reference sample: `samples/zephyr/boards/nordic/system_on_idle/`
