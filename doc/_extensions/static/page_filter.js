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

  let selector = `.hideable:not(.${filters.join(".")})`;
  document.querySelectorAll(selector).forEach(e => e.hidden = true);
};

/**
 * Display classes of the format vX-X-X as clickable tag vX.X.X
 * Also ensure that all relevant elements of the chosen type are hideable.
 * @param {Element} dropdown The related dropdown
 * @param {String}  elementType DOM tag to insert version tags into
 */
function createVersionTags(dropdown, elementType) {
  document.querySelectorAll(elementType).forEach((element) => {
    if (!element.getAttribute("class")) return;

    var versionClasses = element
      .getAttribute("class")
      .split(" ")
      .filter(name => RegExp('v\\d+-\\d+-\\d+').test(name));
    if (versionClasses.length == 0) return;

    if (!element.classList.contains("hideable")) {
      element.classList.add("hideable");
    }

    var versionPara = document.createElement("p");
    versionPara.classList.add("versiontag-container");
    element.insertBefore(versionPara, element.children[1]);

    versionClasses.forEach((className) => {

      var URL = window.location.href.split('?')[0];

      var aTag = document.createElement("a");
      var spanTag = document.createElement("span");

      aTag.setAttribute("href", URL + "?v=" + className);
      spanTag.setAttribute("version", className);
      spanTag.classList.add("versiontag");

      var versionName = className.replace(/v(\d+)-(\d+)-(\d+)/i, 'v$1.$2.$3');
      var textNode = document.createTextNode(versionName);

      /** When clicking a version tag, filter by the corresponding version **/
      spanTag.addEventListener("click", () => displayOption(className, dropdown));

      spanTag.appendChild(textNode);
      aTag.appendChild(spanTag);
      versionPara.appendChild(aTag);
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
 * If versionTagContainer is not None, version tags are created as a
 * child paragraph to all elements with a version class within the
 * specified container element.
 *
 * @param {String} name name attribute identifying the dropdown.
 * @param {String} versionTagContainer container element for version tags.
 */
function setupFiltering(name, versionTagContainer=undefined) {

  ready(() => {

    const dropdown = document.querySelector(`[name='${name}']`);
    all_dropdowns.push(dropdown);

    if (versionTagContainer) {
      createVersionTags(dropdown, versionTagContainer);
    }

    /** When selecting a version from the dropdown, filter **/
    dropdown.addEventListener("change", () => {
      let value = dropdown.options[dropdown.selectedIndex].value;
      displayOption(value, dropdown);
    });

    /** Retrieve the 'v' parameter and switch to that version, if applicable.
        Otherwise, switch to the version that is selected in the dropdown.    **/
    var v = getUrlParameter('v');
    if (versionTagContainer && (RegExp('v\\d+-\\d+-\\d+').test(v))) {
      displayOption(v, dropdown);
    }
    else {
      let value = dropdown.options[dropdown.selectedIndex].value;
      displayOption(value, dropdown);
    };
  });
}

export default setupFiltering;
