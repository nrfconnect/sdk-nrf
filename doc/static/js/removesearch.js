/*
 * When the "Hide Search Matches" (from Sphinx's doctools) link is clicked,
 * rewrite the URL to remove the search term.
 */
$(document).ready(function(){
  $('.highlight-link > a').on('click', function(){
    window.location = window.location.pathname;
  });
});
