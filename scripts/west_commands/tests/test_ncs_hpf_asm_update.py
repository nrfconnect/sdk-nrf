# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import importlib.util
import sys
import textwrap
from pathlib import Path

import pytest

# Load the module by path, as adding scripts/west_commands to sys.path shadows modules
# with the same names (for example utils) imported by other tests.
_spec = importlib.util.spec_from_file_location(
    'ncs_hpf_asm_update', Path(__file__).parents[1] / 'ncs_hpf_asm_update.py'
)
assert _spec and _spec.loader
asm = importlib.util.module_from_spec(_spec)
sys.modules[_spec.name] = asm
_spec.loader.exec_module(asm)


def make_app(root: Path, name: str = 'app', sample_yaml: str = '', hrt: str = 'hrt') -> Path:
    app = root / name
    (app / 'src' / 'hrt').mkdir(parents=True)
    (app / 'CMakeLists.txt').write_text(
        f'hpf_assembly_install(app "${{CMAKE_SOURCE_DIR}}/src/hrt/{hrt}.c")\n'
    )
    (app / 'src' / 'hrt' / f'{hrt}.c').write_text('')
    if sample_yaml:
        (app / 'sample.yaml').write_text(textwrap.dedent(sample_yaml))
    return app


def make_build(
    root: Path, soc: str, files: dict[str, bytes], sources: list[Path] | None = None
) -> Path:
    build_dir = root / 'build' / soc
    build_dir.mkdir(parents=True)
    for name, content in files.items():
        (build_dir / name).write_bytes(content)
    if sources is not None:
        (build_dir / asm.HRT_SOURCES_FILE).write_text(''.join(f'{s}\n' for s in sources))
    return build_dir


def status(target: asm.Target, build_dir: Path, soc: str, check: bool) -> asm.Status:
    return asm.update_asm_files(target, build_dir, soc, check)[0].status


def test_find_hpf_apps(tmp_path):
    make_app(tmp_path, 'gpio')
    make_app(tmp_path, 'mspi')
    other = tmp_path / 'other'
    other.mkdir()
    (other / 'CMakeLists.txt').write_text('project(other)\n')

    assert [app.name for app in asm.find_hpf_apps(tmp_path)] == ['gpio', 'mspi']
    assert asm.find_hpf_apps(tmp_path / 'missing') == []


@pytest.mark.parametrize(
    'board, soc',
    [
        ('nrf54l15dk/nrf54l15/cpuflpr', 'nrf54l15'),
        ('nrf54lm20dk/nrf54lm20b/cpuflpr', 'nrf54lm20b'),
        ('nrf7120dk@1.0.0/nrf7120/cpuflpr', 'nrf7120'),
        ('custom', 'custom'),
    ],
)
def test_soc_from_board(board, soc):
    assert asm.soc_from_board(board) == soc


def test_targets_one_per_soc_with_first_scenario_snippets(tmp_path):
    app = make_app(
        tmp_path,
        sample_yaml='''
            common:
              integration_platforms:
                - nrf54l15dk/nrf54l15/cpuflpr
            tests:
              app.mbox:
                platform_allow:
                  - nrf54l15dk/nrf54l15/cpuflpr
                  - nrf54lm20dk/nrf54lm20a/cpuflpr
                required_snippets:
                  - hpf-gpio-mbox
              app.icmsg:
                platform_allow: nrf54l15dk/nrf54l15/cpuflpr nrf7120dk/nrf7120/cpuflpr
                required_snippets:
                  - hpf-gpio-icmsg
        ''',
    )

    assert asm.targets_from_sample_yaml(app) == [
        asm.Target(app, 'nrf54l15dk/nrf54l15/cpuflpr', 'nrf54l15', ('hpf-gpio-mbox',)),
        asm.Target(app, 'nrf54lm20dk/nrf54lm20a/cpuflpr', 'nrf54lm20a', ('hpf-gpio-mbox',)),
        asm.Target(app, 'nrf7120dk/nrf7120/cpuflpr', 'nrf7120', ('hpf-gpio-icmsg',)),
    ]


def test_targets_merge_common_section(tmp_path):
    app = make_app(
        tmp_path,
        sample_yaml='''
            common:
              platform_allow:
                - nrf54l15dk/nrf54l15/cpuflpr
              required_snippets:
                - common-snippet
            tests:
              app.default:
                platform_allow:
                  - nrf54lv10dk/nrf54lv10a/cpuflpr
        ''',
    )

    assert asm.targets_from_sample_yaml(app) == [
        asm.Target(app, 'nrf54l15dk/nrf54l15/cpuflpr', 'nrf54l15', ('common-snippet',)),
        asm.Target(app, 'nrf54lv10dk/nrf54lv10a/cpuflpr', 'nrf54lv10a', ('common-snippet',)),
    ]


def test_targets_without_sample_yaml(tmp_path):
    assert asm.targets_from_sample_yaml(make_app(tmp_path)) == []


def test_build_command(tmp_path):
    target = asm.Target(tmp_path / 'app', 'nrf54l15dk/nrf54l15/cpuflpr', 'nrf54l15', ('s1',))
    cmd = asm.build_command(target, tmp_path / 'build')

    assert cmd[1:4] == ['-m', 'west', 'build']
    assert cmd[cmd.index('--board') + 1] == 'nrf54l15dk/nrf54l15/cpuflpr'
    assert cmd[cmd.index('--target') + 1] == 'asm_gen'
    assert cmd[cmd.index('--snippet') + 1] == 's1'
    assert '--no-sysbuild' in cmd
    assert cmd[-3:] == [str(tmp_path / 'app'), '--', '-DCONFIG_HPF_DEVELOPER_MODE=y']


def test_update_asm_files_statuses(tmp_path):
    app = make_app(tmp_path)
    hrt_dir = app / 'src' / 'hrt'
    (hrt_dir / 'hrt-nrf54l15.s').write_bytes(b'same\n')
    (hrt_dir / 'hrt-nrf54lm20a.s').write_bytes(b'old\n')
    target = asm.Target(app, 'board', 'soc')
    src = [hrt_dir / 'hrt.c']

    unchanged = make_build(tmp_path / 'a', 'nrf54l15', {'hrt-nrf54l15-temp.s': b'same\r\n'}, src)
    updated = make_build(tmp_path / 'b', 'nrf54lm20a', {'hrt-nrf54lm20a-temp.s': b'new\r\n'}, src)
    added = make_build(tmp_path / 'c', 'nrf54lv10a', {'hrt-nrf54lv10a-temp.s': b'new\n'}, src)

    assert status(target, unchanged, 'nrf54l15', False) is asm.Status.UNCHANGED
    assert status(target, updated, 'nrf54lm20a', False) is asm.Status.UPDATED
    assert status(target, added, 'nrf54lv10a', False) is asm.Status.ADDED
    assert (hrt_dir / 'hrt-nrf54lm20a.s').read_bytes() == b'new\n'
    assert (hrt_dir / 'hrt-nrf54lv10a.s').read_bytes() == b'new\n'


def test_update_asm_files_check_mode_does_not_modify(tmp_path):
    app = make_app(tmp_path)
    asm_file = app / 'src' / 'hrt' / 'hrt-nrf54l15.s'
    asm_file.write_bytes(b'old\n')
    target = asm.Target(app, 'board', 'nrf54l15')
    src = [app / 'src' / 'hrt' / 'hrt.c']

    differs = make_build(tmp_path / 'a', 'nrf54l15', {'hrt-nrf54l15-temp.s': b'new\n'}, src)
    missing = make_build(tmp_path / 'b', 'nrf54lm20a', {'hrt-nrf54lm20a-temp.s': b'new\n'}, src)

    assert status(target, differs, 'nrf54l15', True) is asm.Status.DIFFERS
    assert status(target, missing, 'nrf54lm20a', True) is asm.Status.MISSING
    assert asm_file.read_bytes() == b'old\n'
    assert not (app / 'src' / 'hrt' / 'hrt-nrf54lm20a.s').exists()


def test_update_asm_files_failures(tmp_path):
    app = make_app(tmp_path)
    target = asm.Target(app, 'board', 'nrf54l15')

    no_sources = make_build(tmp_path / 'a', 'nrf54l15', {'hrt-nrf54l15-temp.s': b''})
    not_generated = make_build(tmp_path / 'b', 'nrf54l15', {}, [app / 'src' / 'hrt' / 'hrt.c'])

    result = asm.update_asm_files(target, no_sources, 'nrf54l15', False)[0]
    assert result.status is asm.Status.FAILED
    assert asm.HRT_SOURCES_FILE in result.detail
    result = asm.update_asm_files(target, not_generated, 'nrf54l15', False)[0]
    assert result.status is asm.Status.FAILED
    assert 'hrt-nrf54l15-temp.s' in result.detail


def test_update_asm_files_uses_configured_source_paths(tmp_path):
    app = make_app(tmp_path)
    shared = tmp_path / 'shared'
    shared.mkdir()
    (shared / 'hrt.c').write_text('')
    target = asm.Target(app, 'board', 'nrf54l15')

    build_dir = make_build(
        tmp_path, 'nrf54l15', {'hrt-nrf54l15-temp.s': b'new\n'}, [shared / 'hrt.c']
    )

    result = asm.update_asm_files(target, build_dir, 'nrf54l15', False)[0]
    assert result.status is asm.Status.ADDED
    assert result.asm_file == shared / 'hrt-nrf54l15.s'
    assert (shared / 'hrt-nrf54l15.s').read_bytes() == b'new\n'
    assert not (app / 'src' / 'hrt' / 'hrt-nrf54l15.s').exists()


def test_update_asm_files_name_matches_cmake_name_we(tmp_path):
    app = make_app(tmp_path)
    source = app / 'src' / 'hrt' / 'hrt.core.c'
    target = asm.Target(app, 'board', 'nrf54l15')

    build_dir = make_build(tmp_path, 'nrf54l15', {'hrt-nrf54l15-temp.s': b'new\n'}, [source])

    assert asm.update_asm_files(target, build_dir, 'nrf54l15', False)[0].asm_file == (
        source.parent / 'hrt-nrf54l15.s'
    )


def test_find_duplicate_targets(tmp_path):
    one = asm.Target(tmp_path / 'one' / 'app', 'a/nrf54l15/cpuflpr', 'nrf54l15')
    two = asm.Target(tmp_path / 'two' / 'app', 'a/nrf54l15/cpuflpr', 'nrf54l15')
    other_board = asm.Target(one.app, 'b/nrf54l15/cpuflpr', 'nrf54l15')
    other_soc = asm.Target(one.app, 'a/nrf54lm20a/cpuflpr', 'nrf54lm20a')

    assert asm.find_duplicate_targets([one, two, other_soc]) == {}
    assert asm.find_duplicate_targets([one, two, other_board]) == {
        (one.app, 'nrf54l15'): ['a/nrf54l15/cpuflpr', 'b/nrf54l15/cpuflpr']
    }


def test_build_paths_are_unique_for_apps_with_the_same_name(tmp_path):
    targets = [
        asm.Target(tmp_path / 'one' / 'app', 'board', 'nrf54l15'),
        asm.Target(tmp_path / 'two' / 'app', 'board', 'nrf54l15'),
    ]

    paths = [asm.build_paths(tmp_path, i, t) for i, t in enumerate(targets)]
    assert len({build_dir for build_dir, _ in paths}) == 2
    assert len({log_file for _, log_file in paths}) == 2


def test_find_orphaned_asm_files(tmp_path):
    app = make_app(tmp_path, hrt='hrt-core')
    hrt_dir = app / 'src' / 'hrt'
    known = hrt_dir / 'hrt-core-nrf54l15.s'
    orphan = hrt_dir / 'hrt-core-nrf54h20.s'
    for path in (known, orphan, hrt_dir / 'hrt-core-nrf54l15-temp.s', hrt_dir / 'hrt-x-a.s'):
        path.write_text('')
    target = asm.Target(app, 'board', 'nrf54l15')

    results = [asm.AsmResult(target, asm.Status.UNCHANGED, known)]
    assert asm.find_orphaned_asm_files(results) == [orphan]


def write_sdk(tmp_path: Path, required: str, installed: str | None) -> dict[str, str]:
    zephyr_base = tmp_path / 'zephyr'
    zephyr_base.mkdir()
    (zephyr_base / 'SDK_VERSION').write_text(f'{required}\n')
    sdk_dir = tmp_path / 'sdk'
    sdk_dir.mkdir()
    if installed:
        (sdk_dir / 'sdk_version').write_text(f'{installed}\n')
    return {
        'ZEPHYR_BASE': str(zephyr_base),
        'ZEPHYR_SDK_INSTALL_DIR': str(sdk_dir),
        'ZEPHYR_TOOLCHAIN_VARIANT': 'zephyr',
    }


def test_toolchain_matches(tmp_path):
    assert asm.toolchain_issue(write_sdk(tmp_path, '1.0.1', '1.0.1')) is None


def test_toolchain_version_mismatch(tmp_path):
    issue = asm.toolchain_issue(write_sdk(tmp_path, '1.0.1', '0.17.4'))
    assert issue is not None
    assert '0.17.4' in issue and '1.0.1' in issue


def test_toolchain_unknown_version(tmp_path):
    issue = asm.toolchain_issue(write_sdk(tmp_path, '1.0.1', None))
    assert issue is not None
    assert 'unknown version' in issue


def test_toolchain_not_zephyr_sdk(tmp_path):
    cache = write_sdk(tmp_path, '1.0.1', '1.0.1')
    cache['ZEPHYR_TOOLCHAIN_VARIANT'] = 'gnuarmemb'
    issue = asm.toolchain_issue(cache)
    assert issue is not None
    assert 'gnuarmemb' in issue


def test_read_cmake_cache_and_kconfig(tmp_path):
    (tmp_path / 'CMakeCache.txt').write_text(
        '# comment\n'
        '//help text\n'
        'ZEPHYR_SDK_INSTALL_DIR:PATH=/opt/sdk\n'
        'ZEPHYR_TOOLCHAIN_VARIANT:STRING=zephyr\n'
    )
    (tmp_path / 'zephyr').mkdir()
    (tmp_path / 'zephyr' / '.config').write_text('CONFIG_SOC_SERIES="x"\nCONFIG_SOC="nrf54l15"\n')

    cache = asm.read_cmake_cache(tmp_path)
    assert cache['ZEPHYR_SDK_INSTALL_DIR'] == '/opt/sdk'
    assert cache['ZEPHYR_TOOLCHAIN_VARIANT'] == 'zephyr'
    assert asm.read_kconfig(tmp_path, 'CONFIG_SOC') == 'nrf54l15'
    assert asm.read_kconfig(tmp_path, 'CONFIG_MISSING') is None
