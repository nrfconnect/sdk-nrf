#!/usr/bin/env python3
# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""Desktop viewer for crypto_benchmarks JSON output.

    python3 view_results.py results.json
    python3 view_results.py                  # then File -> Open

Opens a window with one tab per PSA family, plus an overview and a table of
every measurement. Each family tab plots elapsed time on a logarithmic axis and
peak stack beside it. Tall tabs scroll rather than squeezing their labels.

Leading and trailing log lines are tolerated: the first '{' to the last '}' is
parsed, so a raw serial capture usually works unedited.

Requires: matplotlib   (pip install matplotlib)
          tkinter      (Debian/Ubuntu: apt install python3-tk)
"""

import argparse
import json
import statistics
import sys
import time
import tkinter as tk
from collections import defaultdict
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

try:
    import numpy as np
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
    from matplotlib.figure import Figure
    from matplotlib.ticker import FuncFormatter, LogLocator, MaxNLocator, NullFormatter
except ImportError as exc:
    sys.exit(f"This viewer needs matplotlib ({exc}).\n\n    pip install matplotlib\n")

# Stage colours: the first three slots of a CVD-validated categorical palette.
# Worst all-pairs CVD dE 9.2, normal-vision 24.0 against this surface.
STAGE_COLOUR = {"keysetup": "#2a78d6", "single": "#eb6834", "multi": "#1baf7a"}
STAGE_ORDER = ["keysetup", "single", "multi"]

PLANE = "#f9f9f7"
SURFACE = "#fcfcfb"
INK = "#0b0b0b"
INK_2 = "#52514e"
MUTED = "#898781"
GRID = "#e1e0d9"
AXIS = "#c3c2b7"

DPI = 100
FIG_W_PX = 1120          # starting width; tabs then track the window
MIN_FIG_W_PX = 640       # below this the labels stop fitting, so scroll instead
ROW_PX = 22              # vertical room per operation row
PAD_PX = 132             # per-subplot chrome: title + a scale at top AND bottom
MONO = "monospace"


# ---------------------------------------------------------------- formatting
def fmt_time(us):
    if us < 1000:
        return f"{us:.1f} µs"
    if us < 1e6:
        return f"{us / 1000:.2f} ms" if us < 1e4 else f"{us / 1000:.1f} ms"
    return f"{us / 1e6:.2f} s"


def fmt_bytes(b):
    return f"{b / 1024:.1f} KiB" if b >= 1024 else f"{b} B"


def tick_time(us, _pos=None):
    """Axis tick: a real value, never 10^x. Trailing zeros trimmed."""
    if us < 1000:
        v, unit = us, "us"
    elif us < 1e6:
        v, unit = us / 1000.0, "ms"
    else:
        v, unit = us / 1e6, "s"
    text = f"{v:.2f}".rstrip("0").rstrip(".")
    return f"{text} {unit}"


def tick_bytes(b, _pos=None):
    if b < 1024:
        return f"{int(round(b))}"
    text = f"{b / 1024:.1f}".rstrip("0").rstrip(".")
    return f"{text}K"


def op_label(rec):
    """Row identity inside a family tab: the operation, without its stage.

    Stage is the colour, so a single-part and a multipart run of the same
    operation share one row and can be read against each other directly.
    """
    key = f"/{rec['keydesc']}" if rec.get("keydesc") else ""
    return f"{rec['alg']}{key}/{rec['op']}"


def full_label(rec):
    key = f"/{rec['keydesc']}" if rec.get("keydesc") else ""
    return f"{rec['alg']}{key}/{rec['stage']}/{rec['op']}"


def stages_present(rows):
    """Known stages in canonical order, then anything unexpected.

    A stage the sample grows later is plotted in grey rather than dropped.
    """
    seen = {r["stage"] for r in rows}
    return [s for s in STAGE_ORDER if s in seen] + sorted(seen - set(STAGE_ORDER))


# ---------------------------------------------------------------- input
def load(path):
    """Parse a results file. Raises ValueError with a readable message."""
    text = Path(path).read_text()
    start, end = text.find("{"), text.rfind("}")
    if start < 0 or end < start:
        raise ValueError("No JSON object in that file.")
    try:
        doc = json.loads(text[start:end + 1])
    except json.JSONDecodeError as exc:
        raise ValueError(f"Could not parse JSON: {exc}") from exc
    recs = doc.get("operations")
    if not isinstance(recs, list) or not recs:
        raise ValueError("No 'operations' array — is this crypto_benchmarks "
                         "JSON output? Enable CONFIG_CRYPTO_DEMO_OUTPUT_JSON.")
    clean, hidden = [], 0
    for r in recs:
        if not isinstance(r, dict) or not isinstance(r.get("elapsed_us"), (int, float)):
            continue
        # An operation that did not report PSA_SUCCESS measured nothing worth
        # plotting - it is dropped here rather than shown with a status column.
        if r.get("status", 0) != 0:
            hidden += 1
            continue
        if not isinstance(r.get("stack_used"), (int, float)):
            r["stack_used"] = 0
        r.setdefault("stage", "single")
        r.setdefault("group", "other")
        r.setdefault("op", "?")
        r.setdefault("alg", "?")
        clean.append(r)
    if not clean:
        raise ValueError("No operations reported success - nothing to plot.")
    return clean, doc.get("summary") or {}, hidden


# ---------------------------------------------------------------- plotting
def ordered_rows(rows, key):
    """Unique row labels, dearest last so matplotlib draws them at the top."""
    best = defaultdict(float)
    for r in rows:
        best[r["_row"]] = max(best[r["_row"]], r[key])
    return [f for f, _ in sorted(best.items(), key=lambda kv: kv[1])]


def draw_dots(ax, rows, labels, key, log_x, xlabel, title, legend):
    """Horizontal dot plot: one y position per label, one dot per record."""
    index = {lab: i for i, lab in enumerate(labels)}
    ax.set_facecolor(SURFACE)
    picks_x, picks_y, picks_rec = [], [], []

    for stage in stages_present(rows):
        subset = [r for r in rows if r["stage"] == stage]
        if not subset:
            continue
        xs = [max(r[key], 1e-6) if log_x else r[key] for r in subset]
        ys = [index[r["_row"]] for r in subset]
        ax.scatter(xs, ys, s=54, zorder=3,
                   facecolor=STAGE_COLOUR.get(stage, MUTED), alpha=0.9,
                   # a ring in the surface colour keeps overlapping dots legible
                   edgecolor=SURFACE, linewidths=1.6,
                   label=stage if legend else None)
        picks_x += xs
        picks_y += ys
        picks_rec += subset

    if log_x:
        ax.set_xscale("log")
        # Decades, labelled with the value itself rather than 10^n. Minor ticks
        # are drawn but not labelled, so the scale stays readable.
        ax.xaxis.set_major_locator(LogLocator(base=10.0))
        ax.xaxis.set_major_formatter(FuncFormatter(tick_time))
        ax.xaxis.set_minor_locator(LogLocator(base=10.0, subs=(2.0, 5.0), numticks=100))
        ax.xaxis.set_minor_formatter(NullFormatter())
    else:
        ax.xaxis.set_major_locator(MaxNLocator(nbins=8, steps=[1, 2, 5, 10]))
        ax.xaxis.set_major_formatter(FuncFormatter(tick_bytes))

    ax.set_yticks(range(len(labels)))
    ax.set_yticklabels(labels, fontfamily=MONO, fontsize=8.5, color=INK_2)
    ax.set_ylim(-0.8, len(labels) - 0.2)
    # The scale repeats on both edges: a tall tab is readable without scrolling
    # to the far end of it to find out what the axis says.
    ax.tick_params(axis="x", which="both", colors=MUTED, labelsize=8.5,
                   top=True, bottom=True, labeltop=True, labelbottom=True)
    ax.tick_params(axis="y", length=0)
    ax.set_xlabel(xlabel, color=INK_2, fontsize=9)
    ax.set_title(title, color=INK, fontsize=11, loc="left", pad=28)
    ax.grid(True, axis="x", which="major", color=GRID, linewidth=0.8, zorder=0)
    ax.grid(True, axis="x", which="minor", color=GRID, linewidth=0.5, alpha=0.5, zorder=0)
    ax.grid(True, axis="y", color=GRID, linewidth=0.8, alpha=0.6, zorder=0)
    ax.set_axisbelow(True)
    # All four sides: with the scale repeated top and bottom, a frame left open
    # on the right reads as a chart that has been cut off.
    for side in ("left", "bottom", "top", "right"):
        ax.spines[side].set_visible(True)
        ax.spines[side].set_color(AXIS)
    if legend:
        leg = ax.legend(loc="lower right", frameon=True, fontsize=8.5,
                        prop={"family": MONO, "size": 8.5})
        leg.get_frame().set_facecolor(SURFACE)
        leg.get_frame().set_edgecolor(AXIS)
    return picks_x, picks_y, picks_rec


def left_margin(labels, width_px):
    """Fraction of the figure's width to reserve for the y labels.

    A fraction, so it has to be recomputed whenever the window changes size -
    the labels need the same pixels either way.
    """
    longest = max((len(s) for s in labels), default=10)
    return min(0.50, max(0.08, (longest * 6.2 + 26) / max(width_px, 1)))


# ---------------------------------------------------------------- GUI
class ScrollableFigure(ttk.Frame):
    """A figure that fills the window's width and scrolls in its own height.

    Width tracks the viewport so the plot is usable in a small window; height
    stays derived from the row count, so dense tabs scroll instead of squeezing
    their labels together.
    """

    def __init__(self, parent, fig, height_px):
        super().__init__(parent)
        self.fig = fig
        self.height_px = height_px
        self._resize_job = None
        bar = ttk.Frame(self)
        bar.pack(side="top", fill="x")
        self.view = tk.Canvas(self, highlightthickness=0, background=PLANE)
        vsb = ttk.Scrollbar(self, orient="vertical", command=self.view.yview)
        hsb = ttk.Scrollbar(self, orient="horizontal", command=self.view.xview)
        self.view.configure(yscrollcommand=vsb.set, xscrollcommand=hsb.set)
        # Fixed-size edges are packed before the expanding canvas: a widget
        # packed after an expand=True sibling only gets the leftover cavity.
        hsb.pack(side="bottom", fill="x")
        vsb.pack(side="right", fill="y")
        self.view.pack(side="left", fill="both", expand=True)

        self.canvas = FigureCanvasTkAgg(fig, master=self.view)
        widget = self.canvas.get_tk_widget()
        widget.configure(width=FIG_W_PX, height=height_px, borderwidth=0,
                         highlightthickness=0)
        self._pending_scroll = 0
        self._scroll_job = None
        self._scroll_quiet_until = 0.0
        self._window = self.view.create_window(0, 0, anchor="nw", window=widget)
        self.view.configure(scrollregion=(0, 0, FIG_W_PX, height_px))
        NavigationToolbar2Tk(self.canvas, bar)
        self.canvas.draw()

        self.view.bind("<Configure>", self._on_configure)
        self.view.bind("<Map>", lambda e: self.refit())
        self.after_idle(self.refit)

        # Bound on this tab's own widgets, never bind_all: a global binding fires
        # every tab's handler on every notch, and rebinds on each file load.
        # The figure widget covers the viewport, so it needs the binding too.
        for target in (self.view, widget):
            for seq, notches in (("<Button-4>", -1), ("<Button-5>", 1)):
                target.bind(seq, lambda e, n=notches: self._wheel(n), add="+")
            target.bind("<MouseWheel>",
                        lambda e: self._wheel(-1 if e.delta > 0 else 1), add="+")

    def refit(self):
        """Fit to the viewport as it is now (a tab reports width 1 until mapped)."""
        try:
            self._fit(self.view.winfo_width())
        except tk.TclError:
            pass

    def _on_configure(self, event):
        # Coalesce the resize storm: re-rendering a tall figure is not cheap.
        if self._resize_job is not None:
            try:
                self.after_cancel(self._resize_job)
            except (tk.TclError, ValueError):
                pass
        width = event.width
        self._resize_job = self.after(90, lambda: self._fit(width))

    def _fit(self, view_width):
        """Re-render the figure at the viewport's current width."""
        self._resize_job = None
        if not view_width or view_width <= 1:
            return
        width = max(MIN_FIG_W_PX, int(view_width) - 2)
        try:
            # The Tk widget's size is authoritative: FigureCanvasTkAgg resizes the
            # figure AND its backing bitmap from the widget's own <Configure>.
            # Calling set_size_inches() here instead would resize the figure while
            # leaving the bitmap alone, which crops the render.
            widget = self.canvas.get_tk_widget()
            widget.configure(width=width, height=self.height_px)
            self.view.itemconfigure(self._window, width=width, height=self.height_px)
            self.view.configure(scrollregion=(0, 0, width, self.height_px))
            self.relayout(width)
            self.canvas.draw_idle()
        except tk.TclError:
            pass                      # tab torn down mid-resize

    def relayout(self, width_px):
        """Hook: subclasses re-apply width-dependent layout."""

    def _wheel(self, notches):
        """Accumulate notches and reposition once, on idle.

        A fast wheel spin delivers a burst of events; repositioning a tall
        embedded widget once per event is what makes scrolling feel heavy.
        """
        self._pending_scroll += notches
        if self._scroll_job is None:
            # A frame-paced timer rather than after_idle: idle callbacks can fire
            # several times between wheel events, and each reposition of a tall
            # embedded widget is a visible repaint.
            self._scroll_job = self.after(16, self._flush_scroll)
        return "break"

    def _flush_scroll(self):
        self._scroll_job = None
        notches, self._pending_scroll = self._pending_scroll, 0
        if not notches:
            return
        try:
            visible = max(1, self.view.winfo_height())
            step = max(72, int(visible * 0.22))       # a fifth of a screen per notch
            self._scroll_by(notches * step)
        except tk.TclError:
            pass

    def _page(self, direction):
        try:
            # Floor it: an unmapped viewport reports height 1, which would make
            # a page turn scroll nowhere.
            self._scroll_by(direction * max(140, int(self.view.winfo_height() * 0.9)))
        except tk.TclError:
            pass
        return "break"

    def _scroll_by(self, pixels):
        """One absolute reposition, clamped, rather than repeated unit scrolls."""
        # No figure redraw may happen while the widget is being moved: a redraw of
        # a tall figure mid-scroll is what reads as flicker.
        self._scroll_quiet_until = time.monotonic() + 0.12
        span = max(1, self.height_px)
        top = self.view.canvasy(0) + pixels
        visible = max(1, self.view.winfo_height())
        top = max(0, min(top, max(0, span - visible)))
        self.view.yview_moveto(top / span)


class PlotTab(ScrollableFigure):
    """One tab: one or two dot plots, with a hover readout."""

    _hover_warned = False

    def __init__(self, parent, specs):
        # Set before ttk.Frame.__init__ so an early relayout() has them.
        self._labels = [lab for s in specs for lab in s["labels"]]
        self._nspecs = len(specs)
        rows_total = sum(len(s["labels"]) for s in specs)
        height_px = int(rows_total * ROW_PX + PAD_PX * len(specs) + 40)
        fig = Figure(figsize=(FIG_W_PX / DPI, height_px / DPI), dpi=DPI,
                     facecolor=PLANE)
        ratios = [max(1, len(s["labels"])) for s in specs]
        axes = fig.subplots(len(specs), 1, gridspec_kw={"height_ratios": ratios},
                            squeeze=False)[:, 0]

        self.picks = {}
        for ax, spec in zip(axes, specs):
            xs, ys, recs = draw_dots(ax, spec["rows"], spec["labels"], spec["key"],
                                     spec["log"], spec["xlabel"], spec["title"],
                                     spec["legend"])
            self.picks[ax] = (np.asarray(xs, dtype=float),
                              np.asarray(ys, dtype=float), recs)

        # Reserve real pixels for the chrome, then convert to the fractions
        # subplots_adjust wants, so tall and short tabs are spaced alike.
        top_px, bottom_px, gap_px = 60, 54, 116
        axes_px = max(60.0, height_px - top_px - bottom_px - gap_px * (len(specs) - 1))
        self._top_px, self._bottom_px, self._gap_px = top_px, bottom_px, gap_px
        # The rightmost tick label is centred on the axes edge, so half of it sits
        # outside the plot: reserve real pixels for it rather than a fixed fraction.
        self._right_px = 34
        self._axes_px = axes_px
        super().__init__(parent, fig, height_px)
        self.relayout(FIG_W_PX)

        self.annot = {}
        fig.canvas.mpl_connect("motion_notify_event", self._hover)

    def relayout(self, width_px):
        self.fig.subplots_adjust(
            left=left_margin(self._labels, width_px),
            right=1 - self._right_px / max(width_px, 1),
            top=1 - self._top_px / self.height_px,
            bottom=self._bottom_px / self.height_px,
            hspace=self._gap_px / (self._axes_px / self._nspecs))

    def _hover(self, event):
        try:
            if time.monotonic() < self._scroll_quiet_until:
                return          # mid-scroll: leave the canvas alone entirely
            ax = event.inaxes
            if ax is None or ax not in self.picks:
                return self._clear()
            xs, ys, recs = self.picks[ax]
            if not len(xs):
                return
            disp = ax.transData.transform(np.column_stack([xs, ys]))
            d2 = (disp[:, 0] - event.x) ** 2 + (disp[:, 1] - event.y) ** 2
            i = int(d2.argmin())
            if d2[i] > 26 ** 2:
                return self._clear()
            rec = recs[i]
            note = self.annot.get(ax)
            if note is None:
                note = ax.annotate(
                    "", xy=(0, 0), xytext=(14, 14), textcoords="offset points",
                    fontfamily=MONO, fontsize=8.5, color=INK, zorder=6,
                    annotation_clip=False,
                    bbox=dict(boxstyle="square,pad=0.45", facecolor=SURFACE,
                              edgecolor=AXIS, linewidth=0.9))
                self.annot[ax] = note
            note.xy = (xs[i], ys[i])
            text = (f"{full_label(rec)}\n{fmt_time(rec['elapsed_us'])}  ·  "
                    f"{fmt_bytes(rec['stack_used'])}")
            note.set_text(text)
            self._place(note, ax, disp[i], text)
            note.set_visible(True)
            self.canvas.draw_idle()
        except Exception as exc:     # a hover glitch must never kill the window,
            if not PlotTab._hover_warned:   # but it must not be silent either
                PlotTab._hover_warned = True
                print(f"view_results: hover readout disabled ({exc!r})", file=sys.stderr)
            self._clear()

    @staticmethod
    def _place(note, ax, point, text):
        """Flip the readout towards whichever side has room for it.

        Offsetting up-and-right unconditionally puts the box outside the figure
        for a point near the right or top edge, where it is simply not drawn.
        """
        box = ax.get_window_extent()
        width = max(len(line) for line in text.splitlines()) * 5.3 + 22
        height = len(text.splitlines()) * 12 + 14
        px, py = float(point[0]), float(point[1])

        if px + 14 + width > box.x1:
            dx, ha = -14, "right"
        else:
            dx, ha = 14, "left"
        if py + 14 + height > box.y1:
            dy, va = -14, "top"
        else:
            dy, va = 14, "bottom"

        note.xyann = (dx, dy)
        note.set_horizontalalignment(ha)
        note.set_verticalalignment(va)

    def _clear(self):
        changed = False
        for note in self.annot.values():
            if note.get_visible():
                note.set_visible(False)
                changed = True
        if changed:
            self.canvas.draw_idle()


class TableTab(ttk.Frame):
    """Every measurement, sortable - so no value is hover-only."""

    COLS = [("group", "Family", 150), ("alg", "Algorithm", 160),
            ("keydesc", "Key", 200), ("stage", "Stage", 95),
            ("op", "Operation", 130), ("elapsed_us", "Elapsed", 120),
            ("stack_used", "Stack", 110)]

    def __init__(self, parent, recs):
        super().__init__(parent)
        self.recs = recs
        self.sort_key, self.sort_desc = "elapsed_us", True
        keys = [c[0] for c in self.COLS]
        self.tree = ttk.Treeview(self, columns=keys, show="headings", height=26)
        for key, title, width in self.COLS:
            self.tree.heading(key, text=title, command=lambda k=key: self._sort(k))
            self.tree.column(key, width=width, stretch=(key == "keydesc"),
                             anchor="e" if key in ("elapsed_us", "stack_used") else "w")
        vsb = ttk.Scrollbar(self, orient="vertical", command=self.tree.yview)
        self.tree.configure(yscrollcommand=vsb.set)
        vsb.pack(side="right", fill="y")
        self.tree.pack(side="left", fill="both", expand=True)
        self._fill()

    def _sort(self, key):
        self.sort_desc = not self.sort_desc if key == self.sort_key else key in (
            "elapsed_us", "stack_used")
        self.sort_key = key
        self._fill()

    def _fill(self):
        self.tree.delete(*self.tree.get_children())
        def sort_val(r):
            v = r.get(self.sort_key)
            return v if isinstance(v, (int, float)) else str(v or "").lower()
        rows = sorted(self.recs, key=sort_val, reverse=self.sort_desc)
        for r in rows:
            self.tree.insert("", "end", values=(
                str(r["group"]).replace("psa_", ""), r["alg"], r.get("keydesc") or "—",
                r["stage"], r["op"], fmt_time(r["elapsed_us"]),
                fmt_bytes(r["stack_used"])))
        for key, title, _ in self.COLS:
            mark = "" if key != self.sort_key else (" ▾" if self.sort_desc else " ▴")
            self.tree.heading(key, text=title + mark)


class App(tk.Tk):
    def __init__(self, path=None):
        super().__init__()
        self.title("crypto_benchmarks results")
        self.geometry("1280x860")
        self.minsize(560, 420)
        self.configure(background=PLANE)
        self._style()

        top = ttk.Frame(self, style="Plane.TFrame")
        top.pack(side="top", fill="x", padx=18, pady=(14, 6))
        ttk.Label(top, text="crypto_benchmarks results",
                  style="H1.TLabel").pack(side="left")
        ttk.Button(top, text="Open…", command=self.open).pack(side="right")
        self.prov = ttk.Label(self, text="", style="Prov.TLabel")
        self.prov.pack(side="top", fill="x", padx=18, pady=(0, 8))

        self.footnote = ttk.Label(self, style="Note.TLabel", wraplength=1180, text=(
            "Single run per operation, no repetition: read these as orders of magnitude "
            "and relative cost on one board and one configuration, not as benchmark "
            "results. A row times a whole callback rather than one primitive, and stack "
            "is a watermark that includes the callback's own frames."
        ), justify="left")
        self.footnote.pack(side="bottom", fill="x", padx=18, pady=(0, 12))

        # Packed last, so it expands into what is left rather than displacing
        # the footer into the leftover cavity.
        self.nb = ttk.Notebook(self)
        self.nb.pack(side="top", fill="both", expand=True, padx=12, pady=(0, 12))
        self.nb.bind("<<NotebookTabChanged>>", self._tab_changed)
        self.bind("<Configure>", self._reflow)

        # Keyboard scrolling is bound on the window: a Canvas never takes focus,
        # so a key binding on it would never fire.
        self.bind("<Prior>", lambda e: self._scroll_current("page", -1))
        self.bind("<Next>", lambda e: self._scroll_current("page", 1))
        self.bind("<Home>", lambda e: self._scroll_current("home", 0))
        self.bind("<End>", lambda e: self._scroll_current("end", 0))

        if path:
            self.show(path)

    def _current_plot(self):
        try:
            tab = self.nb.nametowidget(self.nb.select())
        except (tk.TclError, KeyError):
            return None
        return tab if isinstance(tab, ScrollableFigure) else None

    def _scroll_current(self, what, direction):
        tab = self._current_plot()
        if tab is None:
            return
        if what == "page":
            tab._page(direction)
        elif what == "home":
            tab.view.yview_moveto(0.0)
        else:
            tab.view.yview_moveto(1.0)
        return "break"

    def _tab_changed(self, _event=None):
        """A tab only learns its real width once it is visible."""
        tab = self._current_plot()
        if tab is not None:
            tab.after_idle(tab.refit)

    def _reflow(self, event):
        if event.widget is self:
            self.footnote.configure(wraplength=max(320, event.width - 48))

    def _style(self):
        st = ttk.Style(self)
        try:
            st.theme_use("clam")
        except tk.TclError:
            pass
        st.configure("Plane.TFrame", background=PLANE)
        st.configure("H1.TLabel", background=PLANE, foreground=INK,
                     font=("TkDefaultFont", 15, "bold"))
        st.configure("Prov.TLabel", background=PLANE, foreground=INK_2,
                     font=("TkFixedFont", 9))
        st.configure("Note.TLabel", background=PLANE, foreground=MUTED,
                     font=("TkDefaultFont", 9))
        st.configure("TNotebook", background=PLANE, borderwidth=0)
        st.configure("TNotebook.Tab", padding=(12, 6))

    def open(self):
        path = filedialog.askopenfilename(
            title="Open crypto_benchmarks JSON",
            filetypes=[("JSON / capture", "*.json *.txt *.log"), ("All files", "*")])
        if path:
            self.show(path)

    def show(self, path):
        try:
            recs, summary, hidden = load(path)
        except (OSError, ValueError) as exc:
            messagebox.showerror("Could not load results", str(exc))
            return
        for tab in self.nb.tabs():
            self.nb.forget(tab)

        total = sum(r["elapsed_us"] for r in recs)
        by_group = defaultdict(list)
        for r in recs:
            by_group[r["group"]].append(r)
        shown = f"{len(recs)} operations"
        if hidden:
            shown += f"  ·  {hidden} hidden (status not 0)"
        self.prov.configure(text=(
            f"{shown}  ·  {len(by_group)} families  ·  {fmt_time(total)} total  ·  {path}"))

        order = sorted(by_group, key=lambda g: -statistics.median(
            [r["elapsed_us"] for r in by_group[g]]))
        for group in order:
            rows = by_group[group]
            for r in rows:
                r["_row"] = op_label(r)
            name = group.replace("psa_", "")
            specs = [
                dict(rows=rows, labels=ordered_rows(rows, "elapsed_us"),
                     key="elapsed_us", log=True, xlabel="elapsed (logarithmic)",
                     title=f"{name} — elapsed time", legend=True),
                dict(rows=rows, labels=ordered_rows(rows, "stack_used"),
                     key="stack_used", log=False, xlabel="peak stack (bytes)",
                     title=f"{name} — peak stack", legend=False),
            ]
            self.nb.add(PlotTab(self.nb, specs), text=name)

        self.nb.add(TableTab(self.nb, recs), text="All measurements")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("json", nargs="?", help="JSON output from crypto_benchmarks")
    args = ap.parse_args()
    App(args.json).mainloop()


if __name__ == "__main__":
    main()
