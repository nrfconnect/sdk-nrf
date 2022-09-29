/*
 * User models for better analysis.
 *
 * How do we know the user model is actually used?
 * The analysis stage should pass --enable-callgraph-metrics.
 * This produces a file called "callgraph-metrics.txt" during analysis.
 * Search for a function there (e.g. assert_post_action), it should
 * list some information that a user model is active.
 */

/*
 * This contains Coverity function declerations e.g. to indicate a kill path
 */
#include <primitives.h>

/*
 * Coverity does not understand that a failed assert
 * stops execution. It therefore continues analysis,
 * which leads to false positives.
 *
 * The function 'assert_post_action' is called on failed asserts,
 * see zephyr/lib/include/sys/__assert.h.
 *
 * Defining here and tying it into the analysis makes
 * Coverity look at this implementation instead,
 * and this implementation tells Coverity to
 * stop analyzing.
 */
 void assert_post_action(char const *, unsigned int)
 {
     /*
      * From coverity/library/primitives.h:
      * "Models a function that aborts an execution path."
      */
     __coverity_panic__();
 }
