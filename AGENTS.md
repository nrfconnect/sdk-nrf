# AGENTS.md

## Cursor Cloud specific instructions

This is the **nRF Connect SDK (NCS)** — an embedded C firmware SDK for Nordic Semiconductor's nRF SoCs, built on top of **Zephyr RTOS**. There are no web servers or databases; the product is compiled firmware.

### West workspace setup

The repo expects to live at `<workspace_root>/nrf/` within a west workspace. Since the Cloud Agent mounts it at `/workspace`, a bind mount is required:

```bash
sudo mkdir -p /home/ubuntu/ncs/nrf
sudo mount --bind /workspace /home/ubuntu/ncs/nrf
cd /home/ubuntu/ncs
west init -l nrf
west update --narrow -o=--depth=1
```

After this, the workspace root is `/home/ubuntu/ncs` and `west` commands should be run from there.

### Environment variables for builds

```bash
export PATH="$HOME/.local/bin:$PATH"
export ZEPHYR_BASE=~/ncs/zephyr
export ZEPHYR_TOOLCHAIN_VARIANT=host   # for native_sim builds
```

### Running Python tests

Per `.github/workflows/scripts-test.yml`:

```bash
python3 -m pytest scripts --ignore=scripts/ci/test_plan.py
```

### Lint

- **ruff**: `ruff check scripts/`
- **pylint**: available via `pylint` (installed from `requirements-fixed.txt`)

### Building firmware (no hardware required)

Use the `native_sim` board target to build and run firmware entirely on the host:

```bash
cd ~/ncs
west build -b native_sim zephyr/samples/hello_world -d /tmp/build_dir
/tmp/build_dir/hello_world/zephyr/zephyr.exe -stop_at=2
```

The built executable is a Linux binary that simulates the Zephyr RTOS kernel locally.

### Key gotchas

- **Symlinks don't work** for the west workspace layout. West resolves symlinks to real paths, so `ln -s /workspace nrf` fails because the parent of `/workspace` is `/` (no write permission). Use `mount --bind` instead.
- **Python dependencies** use a custom Nordic PyPI index (`files.nordicsemi.com`). Always install from `scripts/requirements-fixed.txt` which includes the `--index-url` directive.
- The `requirements-fixed.txt` pins packages for Python >= 3.12. The VM ships with Python 3.12.3 which is compatible.
- **System packages** needed for builds: `ninja-build`, `device-tree-compiler`, `gcc-multilib`, `g++-multilib`, `ccache`.
- `west update --narrow -o=--depth=1` significantly reduces clone time by doing shallow fetches.
