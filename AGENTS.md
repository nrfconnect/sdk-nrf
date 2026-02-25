# AGENTS.md

## Cursor Cloud specific instructions

This is the **nRF Connect SDK (NCS)** — Nordic Semiconductor's embedded firmware SDK built on Zephyr RTOS. There are no web services, databases, or backend APIs to run. The "product" is compiled firmware binaries for Nordic nRF SoCs.

### Workspace layout

The West workspace lives at `/home/ubuntu/ncs/`. The repo at `/workspace` is hardlink-copied to `/home/ubuntu/ncs/nrf/` (required by West's manifest `self.path: nrf`). All West-managed dependencies (Zephyr, MCUboot, mbedTLS, etc.) are under `/home/ubuntu/ncs/`.

### Environment

The Python venv is at `/home/ubuntu/ncs/.venv` and is activated automatically via `~/.bashrc`. Key environment variables (`ZEPHYR_BASE`, `ZEPHYR_SDK_INSTALL_DIR`, `ZEPHYR_TOOLCHAIN_VARIANT`) are also set in `~/.bashrc`.

### Building and running

Build samples/apps for `native_sim` (no hardware needed):

```
cd /home/ubuntu/ncs
west build -b native_sim zephyr/samples/hello_world -d /tmp/build_hello
/tmp/build_hello/hello_world/zephyr/zephyr.exe -stop_at=2
```

For NCS-specific samples (under `nrf/samples/`), use the same pattern:

```
west build -b native_sim nrf/samples/<path> -d /tmp/build_out
```

Not all samples support `native_sim` — check the sample's `sample.yaml` for `platform_allow`.

### Running tests

Use Zephyr's **Twister** test runner for C tests targeting `native_sim`:

```
cd /home/ubuntu/ncs
python zephyr/scripts/twister -T nrf/tests/lib/<test_dir> -p native_sim --short-build-path -O /tmp/twister_out
```

Unity-based tests (under `nrf/tests/unity/`) require Ruby to be installed.

### Linting

- **Python**: `ruff check scripts/` (from `/workspace`)
- **Python static analysis**: `pylint` is available
- **YAML**: `yamllint` is available
- **C formatting**: `clang-format` is available via pip

### Gotchas

- Always run `west` commands from `/home/ubuntu/ncs/`, not from `/workspace` directly.
- `west update` was run with `--narrow -o=--depth=1` for shallow clones. If you need full history for a dependency, run `west update <project>` without these flags.
- The Zephyr SDK minimal package is installed (host tools only). For ARM cross-compilation, you would need to download additional toolchain bundles. `native_sim` builds use the host GCC and work out of the box.
- File changes in `/workspace` are reflected in `/home/ubuntu/ncs/nrf/` because they share hardlinked files.
