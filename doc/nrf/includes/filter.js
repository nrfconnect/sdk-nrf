<script>

  $(document).ready(function() {

    "use strict";

    /** Function that either shows all versions or hides all except one **/
    function displayVersion(theVersion) {

      switch (theVersion) {
        case "all":
          $("#known-issues dl").show();
          break;
        default:
          $("#known-issues dl").hide();
          $("#known-issues dl." + theVersion).show();
      };

      $("[name='versions']").val(theVersion);

    };

    /** Display classes of the format vX-X-X as tag vX.X.X **/
    $("#known-issues dl").each(function() {

      var thisDl = $(this);

      $(this).attr("class").split(" ").forEach(function(className) {

        var URL = window.location.href.split('?')[0];

        if(RegExp('v\\d+-\\d+-\\d+').test(className)) {

          var versionName = className.replace(/v(\d+)-(\d+)-(\d+)/i, 'v$1.$2.$3');
          thisDl.children("dt").append('<a href="' + URL + '?v=' + className + '"><span class="versiontag" version="' + className + '">' + versionName + '</span></a>');

        } else if (className === 'master') {

          thisDl.children("dt").append('<a href="' + URL + '?v=master"><span class="versiontag" version="master">master</span></a>');

        };

      });
    });

    /** When clicking a version tag, filter by the corresponding version **/
    $(".versiontag").click(function() {

      displayVersion($(this).attr("version"));

    });

    /** When selecting a version from the dropdown, filter **/
    $("[name='versions']").change(function() {

      displayVersion($("[name='versions'] option:selected").val());

    });

    /** Function that retrieves parameter from the URL **/
    function getUrlParameter(param) {
      var urlVariables = window.location.search.substring(1).split('&');

      for (var i in urlVariables) {
        var parameterName = urlVariables[i].split('=');

        if (parameterName[0] === param) {
            return parameterName[1] === undefined ? true : decodeURIComponent(parameterName[1]);
        }
      }
    };

    /** Retrieve the 'v' parameter and switch to that version, if applicable.
        Otherwise, switch to the version that is selected in the dropdown.    **/
    var v = getUrlParameter('v');

    if ((RegExp('v\\d+-\\d+-\\d+').test(v)) || v === 'master') {

      displayVersion(v);

    }

    else {

      displayVersion($("[name='versions'] option:selected").val());

    };


  });

</script>
