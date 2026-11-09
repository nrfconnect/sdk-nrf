const all_dropdowns = []

const VERSION_RE = /^v(\d+)-(\d+)-(\d+)(-.*)?$/;

/** Elements with more version tags than this get a compact range view. **/
const COMPACT_THRESHOLD = 2;

function versionLabel(className) {
  return className.replace(/v(\d+)-(\d+)-(\d+)(-.*)?/, (match, p1, p2, p3, p4) => {
    return `v${p1}.${p2}.${p3}${p4 || ''}`;
  });
}

/**
 * Show all options or hide all except one
 * @param {String}  option The option to display
 * @param {Element} dropdown The related dropdown
 */
function displayOption(option, dropdown) {

  dropdown.value = option;

  document.querySelectorAll(".hideable").forEach(e => e.hidden = false);
  var filters = all_dropdowns
    .map(dp => dp.options[dp.selectedIndex].value.toLowerCase())
    .filter(val => val !== "all");

  all_dropdowns.forEach(dp => dp.classList.toggle("pf-active", dp.value !== "all"));

  /** Collapse issue bodies only when nothing is filtered out. **/
  setAllExpanded(filters.length != 0);
  highlightVersionTags(filters);

  if (filters.length != 0) {
    var negativeFilters = filters
      .filter(val => val.charAt(0) === "!")
      .map(val => val.substring(1));
    filters = filters.filter(val => val.charAt(0) !== "!")

    var selector = "";
    if (negativeFilters.length > 0) {
      selector += ".hideable." + negativeFilters.join(",.hideable.");
    }
    if (negativeFilters.length > 0 && filters.length > 0) {
      selector += ",";
    }
    if (filters.length > 0) {
      selector += filters.map(f => `.hideable:not(.${f})`).join(",")
    }

    document.querySelectorAll(selector).forEach(e => e.hidden = true);
  }

  updateCounter();
};

/**
 * Create a single clickable version tag.
 * @param {String[]} versions Version classnames covered by the tag, oldest first.
 * @param {String}   label Displayed text.
 * @param {Element}  dropdown The related dropdown
 */
function createVersionTag(versions, label, dropdown) {
  var URL = window.location.href.split('?')[0];
  var target = versions[versions.length - 1];

  var aTag = document.createElement("a");
  var spanTag = document.createElement("span");

  aTag.setAttribute("href", URL + "?v=" + target);
  spanTag.classList.add("versiontag");
  spanTag.setAttribute("version", target);
  spanTag.dataset.versions = versions.join(" ");
  if (versions.length > 1) {
    spanTag.classList.add("versiontag-range");
    spanTag.title = `${versions.length} versions: ` + versions.map(versionLabel).join(", ");
  }
  /** When clicking a version tag, filter by the corresponding version **/
  spanTag.addEventListener("click", () => displayOption(target, dropdown));

  spanTag.appendChild(document.createTextNode(label));
  aTag.appendChild(spanTag);
  return aTag;
}

/**
 * Group versions into runs of consecutive releases, as ordered in the dropdown.
 *
 * Only plain vX-X-X releases decide whether a run continues. Special builds
 * (vX-X-X-suffix) join the run that spans them, so a build that an issue is not
 * tagged with does not split a range. Versions that are not in the dropdown
 * are left out.
 * @param {String[]} versions Version classnames of one element.
 * @param {Object}   order Position of every dropdown version, oldest first.
 * @param {Object}   releaseOrder Position of every plain release, oldest first.
 * @returns {String[][]} List of runs, newest run first, each run oldest first.
 */
function groupVersionRuns(versions, order, releaseOrder) {
  var runs = [];
  versions
    .filter(v => v in releaseOrder)
    .sort((a, b) => releaseOrder[a] - releaseOrder[b])
    .forEach((v) => {
      var last = runs[runs.length - 1];
      if (last && releaseOrder[v] === releaseOrder[last[last.length - 1]] + 1) {
        last.push(v);
      } else {
        runs.push([v]);
      }
    });

  versions
    .filter(v => v in order && !(v in releaseOrder))
    .forEach((v) => {
      var run = runs.find(r => order[r[0]] < order[v] && order[v] < order[r[r.length - 1]]);
      if (run) {
        run.push(v);
      } else {
        runs.push([v]);
      }
    });

  runs.forEach(run => run.sort((a, b) => order[a] - order[b]));
  return runs.sort((a, b) => order[b[0]] - order[a[0]]);
}

/**
 * Append version tags to a container, as ranges when there are many.
 * @param {Element}  tagDiv Container for the tags.
 * @param {String[]} versions Version classnames of the element.
 * @param {Object}   order Position of every dropdown version, oldest first.
 * @param {Object}   releaseOrder Position of every plain release, oldest first.
 * @param {Element}  dropdown The related dropdown
 */
function appendVersionTags(tagDiv, versions, order, releaseOrder, dropdown) {
  var runs = groupVersionRuns(versions, order, releaseOrder);

  if (versions.length <= COMPACT_THRESHOLD || runs.length == 0) {
    versions.forEach(v => tagDiv.append(createVersionTag([v], versionLabel(v), dropdown)));
    return;
  }

  var compact = document.createElement("span");
  compact.classList.add("versiontag-compact");
  runs.forEach((run) => {
    if (run.length < 3) {
      run.forEach(v => compact.append(createVersionTag([v], versionLabel(v), dropdown)));
    } else {
      var label = `${versionLabel(run[0])} – ${versionLabel(run[run.length - 1])}`;
      compact.append(createVersionTag(run, label, dropdown));
    }
  });

  var full = document.createElement("span");
  full.classList.add("versiontag-full");
  full.hidden = true;
  versions.forEach(v => full.append(createVersionTag([v], versionLabel(v), dropdown)));

  var toggle = document.createElement("button");
  toggle.type = "button";
  toggle.classList.add("versiontag-toggle");
  toggle.setAttribute("aria-expanded", "false");
  toggle.textContent = `all ${versions.length} versions`;
  toggle.addEventListener("click", (event) => {
    event.stopPropagation();
    var showFull = full.hidden;
    full.hidden = !showFull;
    compact.hidden = showFull;
    toggle.setAttribute("aria-expanded", String(showFull));
    toggle.textContent = showFull ? "fewer" : `all ${versions.length} versions`;
  });

  tagDiv.append(compact, full, toggle);
}

/**
 * Mark tags that cover one of the selected versions.
 * @param {String[]} filters Selected dropdown values.
 */
function highlightVersionTags(filters) {
  document.querySelectorAll(".versiontag[data-versions]").forEach((tag) => {
    var versions = tag.dataset.versions.split(" ");
    tag.classList.toggle("selected", filters.some(f => versions.includes(f)));
  });
}

/**
 * Expand or collapse the body of a filterable element.
 * @param {Element} element Element with a dt/dd body.
 * @param {Boolean} open Whether to expand it.
 */
function setExpanded(element, open) {
  element.classList.toggle("pf-open", open);
  element.querySelector(":scope > dt").setAttribute("aria-expanded", String(open));
  element.querySelectorAll(":scope > dd").forEach((dd) => {
    if (open) {
      dd.removeAttribute("hidden");
    } else {
      /** "until-found" keeps the text searchable with Ctrl+F in supporting browsers. **/
      dd.setAttribute("hidden", "until-found");
    }
  });
}

function setAllExpanded(open) {
  document.querySelectorAll(".pf-collapsible").forEach(e => setExpanded(e, open));
}

/**
 * Make the dt of a dt/dd element toggle the visibility of its dd.
 * @param {Element} element The filterable element.
 */
function makeCollapsible(element) {
  var dt = element.querySelector(":scope > dt");
  if (!dt || !element.querySelector(":scope > dd")) return;

  element.classList.add("pf-collapsible", "pf-open");
  dt.setAttribute("role", "button");
  dt.setAttribute("tabindex", "0");
  dt.setAttribute("aria-expanded", "true");

  var toggle = () => setExpanded(element, !element.classList.contains("pf-open"));
  dt.addEventListener("click", (event) => {
    if (event.target.closest("a, button")) return;
    toggle();
  });
  dt.addEventListener("keydown", (event) => {
    if (event.target !== dt || (event.key !== "Enter" && event.key !== " ")) return;
    event.preventDefault();
    toggle();
  });
  element.querySelectorAll(":scope > dd").forEach((dd) => {
    dd.addEventListener("beforematch", () => setExpanded(element, true));
  });
}

function expandHashTarget() {
  var id = decodeURIComponent(window.location.hash.substring(1));
  var target = id && document.getElementById(id);
  var element = target && target.closest(".pf-collapsible");
  if (element) setExpanded(element, true);
}

/**
 * Insert "Expand all", "Collapse all" and a visible-item counter after the last
 * dropdown that shares a parent with the given dropdown.
 * @param {Element} dropdown The related dropdown
 */
function createToolbar(dropdown) {
  if (document.querySelector(".pf-toolbar") || !document.querySelector(".pf-collapsible")) return;

  var toolbar = document.createElement("span");
  toolbar.classList.add("pf-toolbar");

  [["Expand all", true], ["Collapse all", false]].forEach(([label, open]) => {
    var button = document.createElement("button");
    button.type = "button";
    button.textContent = label;
    button.addEventListener("click", () => setAllExpanded(open));
    toolbar.append(button);
  });

  var counter = document.createElement("span");
  counter.classList.add("pf-counter");
  counter.setAttribute("aria-live", "polite");
  toolbar.append(counter);

  var siblings = dropdown.parentElement.querySelectorAll(":scope > .dropdown-select");
  siblings[siblings.length - 1].after(toolbar);
}

function updateCounter() {
  var counter = document.querySelector(".pf-counter");
  if (!counter) return;
  var all = document.querySelectorAll(".pf-collapsible");
  var visible = Array.from(all).filter(e => !e.hidden).length;
  counter.textContent = `Showing ${visible} of ${all.length}`;
}

/**
 * Add filter tags to a selected DOM element.
 *
 * Display classes of the format vX-X-X as clickable tag vX.X.X, collapsed into
 * ranges of consecutive versions when there are many.
 * Also ensure that all relevant elements of the chosen type are hideable.
 * @param {Element} dropdown The related dropdown
 * @param {String}  elementType DOM tag to insert version tags into
 * @param {Object}  filterTags Mapping of classnames to displaynames of filter tags.
 */
function createFilterTags(dropdown, elementType, filterTags) {
  var classFilterRE = Object.keys(filterTags);
  var index = classFilterRE.indexOf("versions");
  if (index !== -1) {
      classFilterRE[index] = "v\\d+-\\d+-\\d+(-.*)?";
  }

  /** Classnames are lowercase, but dropdown values can contain uppercase letters. **/
  var dropdownVersions = Array.from(dropdown.options)
    .map(o => o.value.toLowerCase())
    .filter(v => VERSION_RE.test(v))
    .reverse();
  var order = {};
  dropdownVersions.forEach((v, i) => order[v] = i);
  var releaseOrder = {};
  dropdownVersions
    .filter(v => !VERSION_RE.exec(v)[4])
    .forEach((v, i) => releaseOrder[v] = i);

  var parentElements = elementType.split("/");
  document.querySelectorAll(parentElements.shift()).forEach((element) => {
    if (!element.getAttribute("class")) return;
      var filterClasses = element
      .getAttribute("class")
      .split(" ")
      .filter(name => RegExp(classFilterRE.join("|")).test(name));
    if (filterClasses.length == 0) return;

    if (!element.classList.contains("hideable")) {
      element.classList.add("hideable");
    }

    if (!element.classList.contains("simple")) {
      element.classList.add("simple");
    }

    makeCollapsible(element);

    var containerParent = element;
    for (var containerElement of parentElements) {
      var containerChild = containerParent.querySelector(containerElement);
      if (containerChild == null) return;
      containerParent = containerChild;
    }

    var tagDiv = document.createElement("div");
    tagDiv.classList.add("filtertag-container");
    containerParent.append(tagDiv);

    filterClasses
      .filter(className => className in filterTags)
      .forEach((className) => {
        var aTag = document.createElement("a");
        var spanTag = document.createElement("span");
        spanTag.setAttribute("filter", className);
        spanTag.classList.add("filtertag");
        spanTag.appendChild(document.createTextNode(filterTags[className]));
        aTag.appendChild(spanTag);
        tagDiv.appendChild(aTag);
      });

    var versions = filterClasses.filter(className => VERSION_RE.test(className));
    if (versions.length > 0) {
      appendVersionTags(tagDiv, versions, order, releaseOrder, dropdown);
    }
  });

  createToolbar(dropdown);
}

/**
 * Function that retrieves parameter from the URL
 * @param {String} param The requested URL parameter
 */
function getUrlParameter(param) {
  var urlVariables = window.location.search.substring(1).split('&');

  for (var i in urlVariables) {
    var parameterName = urlVariables[i].split('=');

    if (parameterName[0] === param) {
        return parameterName[1] === undefined ? true : decodeURIComponent(parameterName[1]);
    }
  }
};

var ready = (callback) => {
  if (document.readyState != "loading") callback();
  else document.addEventListener("DOMContentLoaded", callback);
}

/**
 * Set up appropriate event listener for dropdown with attribute name.
 * If filterTagContainer is not None, filter tags are created as a
 * child paragraph to all elements with a version class within the
 * specified container element.
 *
 * @param {String} name name attribute identifying the dropdown.
 * @param {String} filterTagContainer container element for version tags.
 */
function setupFiltering(name, filterTagContainer=undefined, filterTags={}) {

  ready(() => {

    const dropdown = document.querySelector(`[name='${name}']`);
    all_dropdowns.push(dropdown);

    if (filterTagContainer) {
      createFilterTags(dropdown, filterTagContainer, filterTags);
    }

    /** When selecting a version from the dropdown, filter **/
    dropdown.addEventListener("change", () => {
      var value = dropdown.options[dropdown.selectedIndex].value;
      displayOption(value, dropdown);
    });

    /** Retrieve the 'v' parameter and switch to that version, if applicable.
        Otherwise, switch to the version that is selected in the dropdown.    **/
    var v = getUrlParameter('v');
    if ("versions" in filterTags && (RegExp('v\\d+-\\d+-\\d+(-.*)?').test(v))) {
      displayOption(v, dropdown);
    }
    else {
      var value = dropdown.options[dropdown.selectedIndex].value;
      displayOption(value, dropdown);
    };

    expandHashTarget();
    window.addEventListener("hashchange", expandHashTarget);
  });
}

export default setupFiltering;
