function NCS () {
  "use strict";

  let state = {};

  // XXX: do not remove the trailing '/'
  const NCS_PATH_PREFIX = "/nRF_Connect_SDK/doc/";

  /*
   * Allow running from localhost; local build can be served with:
   * python -m http.server
   */
  state.updateLocations = function(){
    const host = window.location.host;
    if (host.startsWith("localhost")) {
      this.url_prefix = "/";
    } else {
      this.url_prefix = NCS_PATH_PREFIX;
    }

    this.url_root = window.location.protocol + "//" + host + this.url_prefix;
    this.url_suffix = "/nrf/index.html";
    this.in_dev_url = this.url_root + "latest" + this.url_suffix;
    this.version_data_url = this.url_root + "latest" + "/versions.json";
  };

  /*
   * Infer the currently running version of the documentation
   */
  state.findCurrentVersion = function() {
    const path = window.location.pathname;
    if (path.startsWith(this.url_prefix)) {
      const prefix_len = this.url_prefix.length;
      window.NCS.current_version = path.slice(prefix_len).split("/")[0];
    } else {
      window.NCS.current_version = "latest";
    }
  };

  /*
   * This updates the list of versions displayed below the current version.
   */
  state.updateVersionDropDown = function() {
    const ncs = window.NCS;
    const versions = ncs.versions.VERSIONS;

    // Avoid applying DOM changes on errors
    const path = window.location.pathname;
    if (!path.startsWith(this.url_prefix)) {
      return;
    }

    // Only display dropdown for nRF Connect documentation set
    const prefix_len = this.url_prefix.length;
    if (path.slice(prefix_len).split("/")[1] !== "nrf") {
      return;
    }

    let links = '';
    $.each(versions, function(_, v) {
      if (v !== ncs.current_version) {
        links += "<li><a href=" + ncs.url_root + v + ncs.url_suffix + ">" +
                  v + "</a></li>";
      }
    });

    $("div.version").append("<span class='fa fa-caret-down dropit'></span>" +
      "<ul class='dropdown'>" + links + "</ul>");

    // hide version dropdown by default & toggle
    $(".dropdown").hide();
    $(".dropit").click(function(){
      $(this).next(".dropdown").toggle();
    });
  };

  /*
   * Display a message at the top of the page to inform the user that the
   * version currently being browsed is not the latest.
   */
  state.showVersion = function() {
    const ncs = window.NCS;
    const VERSIONS = ncs.versions.VERSIONS;
    const latest_release_url = ncs.url_root + VERSIONS[1] + ncs.url_suffix;

    const OLD_RELEASE_MSG =
      "You are looking at an older version of the documentation. You might " +
      "want to switch to the documentation for the <a href='" +
      latest_release_url + "'>" + VERSIONS[1] + "</a> release or the " +
      "<a href='" + ncs.in_dev_url + "'>current state of development</a>.";

    const LATEST_RELEASE_MSG =
      "You are looking at the documentation for the last official release.";

    if ($("#version_status").length === 0) {
      $("div[role='navigation'] > ul.wy-breadcrumbs").
        after("<div id='version_status'></div>");
    }

    switch (ncs.current_version) {
    case VERSIONS[0]:
      $("div#version_status").hide();
      break;
    case VERSIONS[1]:
      $("div#version_status").html(LATEST_RELEASE_MSG);
      break;
    default:
      $("div#version_status").html(OLD_RELEASE_MSG);
    }
  };

  /*
   * Update the versions displayed in the documentation chooser
   */
  state.updateDocsetVersions = function() {
    const ncs = window.NCS;
    const current_version = ncs.current_version;
    const by_version = ncs.versions.COMPONENTS_BY_VERSION[current_version];

    let found = {};
    $.each(by_version, function(docset) {
      found[docset] = false;
    });

    // Update all versions but the one that is selected
    $('div.rst-other-versions').children('div').each(function (_, el) {
      $.each(by_version, function(docset, version) {
        if ($(el).hasClass(docset)){
          let a = $(el).children('a');
          a.text(a.text() + ' ' + version);
          found[docset] = true;
        }
      });
    });

    // Update the currently selected version
    $('span.rst-current-version > span.fa-book').each(function (_, el) {
      $.each(found, function(name, value) {
        if (!value) {
          $(el).text($(el).text() + ' ' + by_version[name]);
          return;
        }
      });
    });
  };

  state.updatePage = function() {
    let ncs = window.NCS;
    ncs.findCurrentVersion();
    ncs.updateVersionDropDown();
    ncs.showVersion();
    ncs.updateDocsetVersions();
  };

  const NCS_SESSION_KEY = "ncs";

  /*
   * Load a versions.json from the session cache if available
   */
  state.loadVersions = function() {
    let versions_data = window.sessionStorage.getItem(NCS_SESSION_KEY);
    if (versions_data) {
      window.NCS.versions = JSON.parse(versions_data);
      return true;
    }
    return false;
  }

  /*
   * Update the session cache with a new versions.json
   */
  state.saveVersions = function(versions_data) {
    let ncs = window.NCS;
    const session_value = JSON.stringify(versions_data);
    window.sessionStorage.setItem(NCS_SESSION_KEY, session_value);
    window.NCS.versions = versions_data;
  }

  /*
   * When the "Hide Search Matches" (from Sphinx's doctools) link is clicked,
   * rewrite the URL to remove the search term.
   */
  state.hideSearchMatches = function() {
    $('.highlight-link > a').on('click', function(){
      // Remove any ?highlight=* search querystring element
      window.location.assign(
        window.location.href.replace(/[?]highlight=[^#]*/, '')
      );
    });
  }

  return state;
};

if (typeof window !== 'undefined') {
  window.NCS = NCS();
}

$(document).ready(function(){
  window.NCS.updateLocations();
  window.NCS.hideSearchMatches();

  if (window.NCS.loadVersions()) {
    window.NCS.updatePage();
  } else {
    /* Get versions file from remote server. */
    $.getJSON(window.NCS.version_data_url, function(json_data) {
      window.NCS.saveVersions(json_data);
      window.NCS.updatePage();
    });
  }
});
