const all_dropdowns = []

/**
 * Show all options or hide all except one
 * @param {String}  option The option to display
 * @param {Element} dropdown The related dropdown
 */
function displayOption(option, dropdown) {

  dropdown.value = option;

  document.querySelectorAll(".hideable").forEach(e => e.hidden = false);
  var filters = all_dropdowns
    .map(dp => dp.options[dp.selectedIndex].value)
    .filter(val => val !== "all");

  if (filters.length == 0) return;

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
};

/**
 * Add filter tags to a selected DOM element.
 *
 * Display classes of the format vX-X-X as clickable tag vX.X.X
 * Also ensure that all relevant elements of the chosen type are hideable.
 * @param {Element} dropdown The related dropdown
 * @param {String}  elementType DOM tag to insert version tags into
 * @param {Object}  filterTags Mapping of classnames to displaynames of filter tags.
 */
function createFilterTags(dropdown, elementType, filterTags) {
  var classFilterRE = Object.keys(filterTags);
  var index = classFilterRE.indexOf("versions");
  if (index !== -1) {
      classFilterRE[index] = "v\\d+-\\d+-\\d+";
  }
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

    var containerParent = element;
    for (var containerElement of parentElements) {
      var containerChild = containerParent.querySelector(containerElement);
      if (containerChild == null) return;
      containerParent = containerChild;
    }

    var tagDiv = document.createElement("div");
    tagDiv.classList.add("filtertag-container");
    containerParent.append(tagDiv);

    filterClasses.forEach((className) => {

      var URL = window.location.href.split('?')[0];

      var aTag = document.createElement("a");
      var spanTag = document.createElement("span");

      var filterName;
      if (className in filterTags) {
        spanTag.setAttribute("filter", className);
        spanTag.classList.add("filtertag");
        filterName = filterTags[className];
      }
      else if (RegExp('v\\d+-\\d+-\\d+').test(className)) {
        aTag.setAttribute("href", URL + "?v=" + className);
        spanTag.setAttribute("version", className);
        spanTag.classList.add("versiontag");
        filterName = className.replace(/v(\d+)-(\d+)-(\d+)/i, 'v$1.$2.$3');
        /** When clicking a version tag, filter by the corresponding version **/
        spanTag.addEventListener("click", () => displayOption(className, dropdown));
      }
      var textNode = document.createTextNode(filterName);

      spanTag.appendChild(textNode);
      aTag.appendChild(spanTag);
      tagDiv.appendChild(aTag);
    });
  });
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
    if ("versions" in filterTags && (RegExp('v\\d+-\\d+-\\d+').test(v))) {
      displayOption(v, dropdown);
    }
    else {
      var value = dropdown.options[dropdown.selectedIndex].value;
      displayOption(value, dropdown);
    };
  });
}

export default setupFiltering;
