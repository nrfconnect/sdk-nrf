#!/usr/bin/env python3

import json
import re
from collections import defaultdict
import sqlite3


def sanitize_kconfig_filename(e: str) -> str:
    parts = e.split("/")

    # big modules are split
    for k, v in [("<module:nrf>", "nrf"), ("<module:nrfxlib>", "nrfxlib")]:
        if e.startswith(k):
            return f'{".".join([v,*parts[1:-1]])}.html'

    # small modules are merged
    m = re.match(r"<module:(.*?)>", e)
    if m:
        return f"{m.groups()[0]}.html"

    # single files like "Kconfig.zephyr"
    m = re.match(r"^Kconfig\.(.*)", e)
    if m:
        return f"{m.groups()[0]}.html"

    # zephyr module top-level files
    m = re.match(r"modules/Kconfig.(.*)$", e)
    if m:
        return f"zephyr.modules.{m.groups()[0]}.html"

    # these should be zephyr entries
    if len(parts) > 1:
        return f'zephyr.{".".join(parts[:-1])}.html'

    # hopefully this is not needed
    return f"{e}.html"


def kconfig_option_to_html(e: dict):
    result = ""
    result += f'<dl class="kconfig" id="{e["name"]}">'
    result += f'<dt class="sig sig-object"><span class="pre">{e["name"]}</span></dt>'
    result += '<dd class="entry">'

    if e["prompt"]:
        result += f'<p><em>{e["prompt"]}</em></p>'
    if e["help"]:
        result += f'<p>{e["help"]}</p>'

    result += '<dl class="field-list simple">'
    if e["type"]:
        result += f'<dt>Type</dt><dd><code class="docutils literal"><span class="pre">{e["type"]}</span></code></dd>'

    for k, v in listings:
        if e[k]:
            result += f'<dt>{v}</dt><dd><ul class="simple">'
            for d in e[k]:
                result += f"<li>{d}</li>"
            result += "</ul><dd>"

    if e["filename"] and e["filename"]:
        result += f'<dt>Location</dt><dd><code class="docutils literal"><span class="pre">{e["filename"]}:{e["linenr"]}</span></code></dd>'

    if e["menupath"]:
        result += f'<dt>Menu path</dt><dd><code class="docutils literal"><span class="pre">{e["menupath"]}</span></code></dd>'

    result += "</dl>"
    result += "</dd>"
    result += "</dl>"
    return result


def option_to_href(m) -> str:
    if m.group(1) in option_to_module:
        return f'<a href="{option_to_module[m.group(1)]}#{m.group(1)}">{m.group(1)}</a>'
    return m.group(1)


def option_to_href_li(m) -> str:
    return f"<li>{option_to_href(m)}</li>"


with open("index.html", "r") as f:
    original_index = f.read()
body_start = '<body class="wy-body-for-nav">'
pos = original_index.find(body_start)
header = original_index[:pos] + body_start + "<div>"
footer = "</div></body></html>"

with open("kconfig.json", "r") as f:
    kconfig_db = json.load(f)

listings = [
    ("dependencies", "Dependencies"),
    ("defaults", "Defaults"),
    ("alt_defaults", "Defaults"),
    ("ranges", "Ranges"),
    ("choices", "Choices"),
    ("selects", "Selects"),
    ("selected_by", "Selected by"),
    ("implies", "Implies"),
    ("implied_by", "Implied by"),
]

filenames = set()

# kconfig_db sanitization
for e in kconfig_db:
    e["name"] = e["name"].upper()
    if e["filename"].startswith("../nrf/subsys"):
        e["filename"] = e["filename"].replace("../nrf", "<module:nrf>")

    filenames.add(e["filename"])
    for k, v in listings:
        if not e[k]:
            continue
        if type(e[k]) != list:
            e[k] = [e[k]]
        for i, d in enumerate(e[k]):
            if type(d) == str and d.startswith("CONFIG"):
                d = f'<a href="#{d}">{d}</a>'
            if k == "alt_defaults":
                e[k][i] = f'{d[0]}<em> at </em><code class="docutils literal"><span class"pre"="">{d[1]}</span></code>'

filename_to_module = {}
for e in filenames:
    filename_to_module[e] = sanitize_kconfig_filename(e)


option_to_module = {}
for e in kconfig_db:
    option_to_module[e["name"]] = filename_to_module[e["filename"]]

html_files = defaultdict(str)
for e in kconfig_db:
    html_str = kconfig_option_to_html(e)
    html_str = re.sub(r'<a href="#(.*?)">.*?<\/a>', option_to_href, html_str)
    html_str = re.sub(r"<li>(CONFIG_.*?)<\/li>", option_to_href_li, html_str)
    html_files[filename_to_module[e["filename"]]] += html_str

for k, v in html_files.items():
    with open(k, "w") as f:
        f.write(header + v + footer)

with open("index.html", "w") as f:
    f.write(header)
    for e in kconfig_db:
        f.write(f'<p><a href="{option_to_module[e["name"]]}#{e["name"]}">{e["name"]}</a></p>')
    f.write(footer)

# add some style
with open("_static/css/kconfig.css", "a") as f:
    f.write("""
dt.sig {
    display: table;
    margin: 6px 0;
    font-size: 90%;
    line-height: normal;
    background: #e7f2fa;
    color: #2980b9;
    border-top: 3px solid #6ab0de;
    padding: 6px;
    position: relative;
}

dd.entry {
    margin: 0 0 12px 24px;
    line-height: 24px;
}
""")

# fix DB

db_conn = sqlite3.connect("../docSet.dsidx")
cur = db_conn.cursor()
cur2 = db_conn.cursor()
res = cur.execute("select * from searchIndex")

for id, name, t, path in res:
    if not name.startswith("CONFIG"):
        cur2.execute(f"delete from searchIndex where id={id}")
        continue
    name = name.upper()
    path = f"{option_to_module[name]}#{name}"
    cur2.execute(f'update searchIndex set path = "{path}" where id={id}')
    cur2.execute(f'update searchIndex set name = "{name}" where id={id}')

db_conn.commit()
