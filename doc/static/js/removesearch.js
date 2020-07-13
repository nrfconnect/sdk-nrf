/*
 * When the "Hide Search Matches" (from Sphinx's doctools) link is clicked,
 * rewrite the URL to remove the search term.
 */
$(document).ready(function(){
  $('.highlight-link > a').on('click', function(){
    // Remove any ?highlight=* search querystring element
    window.location.assign(
      window.location.href.replace(/[?]highlight=[^#]*/, '')
    );
  });
});
