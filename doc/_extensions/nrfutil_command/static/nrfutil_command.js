/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * The example values switch and the operating system buttons of nrfutil-command blocks.
 *
 * The build renders a labelled copy of the block for each operating system, so that the values of
 * all systems are in the HTML. The buttons show the copy of the selected system.
 *
 * The copy button copies the text that the block shows, so it copies the placeholders, the example
 * values or the values that the reader typed, depending on the state of the switch.
 */

(() => {
  const OS_KEY = "nrfutil-command-os";
  const OS_LABELS = { windows: "Windows", linux: "Linux", macos: "macOS" };
  // The class that Pygments gives the placeholder token of the extension's lexers
  const PLACEHOLDER_SELECTOR = "pre span.ge-Placeholder";
  // The copies of the block with the values of each operating system, which the build renders
  const VARIANT_SELECTOR = "div.nrfutil-command-os-variant";

  const blocks = [];

  const load = (key) => {
    try {
      return localStorage.getItem(key);
    } catch {
      return null;
    }
  };

  const save = (key, value) => {
    try {
      localStorage.setItem(key, value);
    } catch {
      // Storage can be disabled. The state then lasts until the page is closed.
    }
  };

  const detectOs = () => {
    const platform = (navigator.userAgentData?.platform || navigator.platform || "").toLowerCase();
    if (platform.startsWith("win")) return "windows";
    if (platform.startsWith("mac")) return "macos";
    return "linux";
  };

  const makeEditable = (element) => {
    element.contentEditable = "plaintext-only";
    if (element.contentEditable !== "plaintext-only") element.contentEditable = "true";
    element.setAttribute("role", "textbox");
    element.setAttribute("aria-label", `Value of ${element.dataset.name}`);
    element.spellcheck = false;
  };

  const makeStatic = (element) => {
    element.removeAttribute("contenteditable");
    element.removeAttribute("role");
    element.removeAttribute("aria-label");
  };

  const render = (block) => {
    const showExamples = block.toggle?.checked ?? false;
    for (const variant of block.variants) {
      variant.hidden = !variant.dataset.os.split(" ").includes(block.os);
    }
    for (const element of block.placeholders) {
      const name = element.dataset.name;
      if (element === document.activeElement) continue;
      if (showExamples) {
        element.textContent = block.edited[name] ?? block.values[name] ?? `<${name}>`;
        element.classList.add("nrfutil-editable");
        makeEditable(element);
      } else {
        element.textContent = `<${name}>`;
        element.classList.remove("nrfutil-editable");
        makeStatic(element);
      }
    }
  };

  const setAll = (key, value, apply) => {
    save(key, value);
    for (const block of blocks) {
      apply(block);
      render(block);
    }
  };

  // Each switch starts in the state that its block sets, so the choice isn't stored across pages.
  // Changing one switch changes all the switches on the page.
  const createToggle = (checked) => {
    const label = document.createElement("label");
    label.className = "nrfutil-command-switch";
    const input = document.createElement("input");
    input.type = "checkbox";
    input.setAttribute("role", "switch");
    input.checked = checked;
    label.append(input, " Show example values");
    input.addEventListener("change", () => {
      for (const block of blocks) {
        if (block.toggle) block.toggle.checked = input.checked;
        render(block);
      }
    });
    return { label, input };
  };

  const selectOs = (block, os) => {
    if (!block.osButtons.some((button) => button.value === os)) return;
    block.os = os;
    for (const button of block.osButtons) {
      button.setAttribute("aria-pressed", String(button.value === os));
    }
  };

  const createOsButtons = (block, systems) => {
    const group = document.createElement("div");
    group.className = "nrfutil-command-os";
    group.setAttribute("role", "group");
    group.setAttribute("aria-label", "Operating system");
    block.osButtons = systems.map((os) => {
      const button = document.createElement("button");
      button.type = "button";
      button.value = os;
      button.textContent = OS_LABELS[os] || os;
      button.addEventListener("click", () =>
        setAll(OS_KEY, os, (other) => selectOs(other, os)),
      );
      group.append(button);
      return button;
    });
    const preferred = load(OS_KEY) || detectOs();
    selectOs(block, systems.includes(preferred) ? preferred : systems[0]);
    return group;
  };

  const setUp = (controls) => {
    const element = controls.closest("div.nrfutil-command");
    if (!element) return;
    const block = {
      values: JSON.parse(controls.dataset.values || "{}"),
      variants: [...element.querySelectorAll(VARIANT_SELECTOR)],
      placeholders: [...element.querySelectorAll(PLACEHOLDER_SELECTOR)],
      edited: {},
      toggle: null,
      os: null,
      osButtons: [],
    };
    for (const placeholder of block.placeholders) {
      placeholder.dataset.name = placeholder.textContent.slice(1, -1);
    }

    const systems = Object.keys(OS_LABELS).filter((os) =>
      block.variants.some((variant) => variant.dataset.os.split(" ").includes(os)),
    );
    if (systems.length) {
      controls.append(createOsButtons(block, systems));
      // The buttons name the shown system, so the labels are only for readers without scripts
      element.classList.add("nrfutil-command-os-switched");
    }

    if (block.placeholders.length) {
      const { label, input } = createToggle(!("exampleToggleOff" in controls.dataset));
      block.toggle = input;
      controls.append(label);
    }

    for (const placeholder of block.placeholders) {
      placeholder.addEventListener("keydown", (event) => {
        if (event.key === "Enter") event.preventDefault();
      });
      placeholder.addEventListener("input", () => {
        const name = placeholder.dataset.name;
        block.edited[name] = placeholder.textContent;
        for (const other of block.placeholders) {
          if (other !== placeholder && other.dataset.name === name) {
            other.textContent = placeholder.textContent;
          }
        }
      });
    }

    blocks.push(block);
    render(block);
  };

  const start = () => document.querySelectorAll("div.nrfutil-command-controls").forEach(setUp);

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", start);
  } else {
    start();
  }
})();
