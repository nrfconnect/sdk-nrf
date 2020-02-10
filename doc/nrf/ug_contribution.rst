.. _ug_contribution:

Contributing to the |NCS|
*************************

The |NCS| is maintained by Nordic Semiconductor, but it is open to community contributions.

As a general practice, we encourage to make small, controlled changes.
This practice simplifies reviewing, merging, and rebasing.
In addition, it keeps the change history clear and clean.

When contributing to the |NCS|, you should provide as much information as you can about your change.
Before submitting, you must also update the appropriate documentation and test your changes thoroughly.

The general GitHub workflow that is used by |NCS| developers is based on a combination of Git commands on the command line and browser interaction with GitHub.
As usual with Git, there are multiple ways of getting a task done.
The following steps show a typical workflow:

1. :ref:`dm-wf-fork`.
   The following substeps summarize the most important commands to start working with a fork of the repository:

   a. Fork the fw-nrfconnect-nrf repository to your personal account on GitHub.
      To do so, click the `fork button on the fw-nrfconnect-nrf repository page`_ (upper right corner).

   #. On your computer, change into the :file:`nrf` folder that was created when you :ref:`obtained the code <cloning_the_repositories_win>`::

         cd ncs/nrf

      Rename the default remote that points to the `upstream nRF Connect SDK repository <fw-nrfconnect-nrf_>`_ from ``origin`` to ``ncs``::

         git remote rename origin ncs

      Let Git know about the fork that you just created and name it ``origin``.
      Replace *your_github_id* with your GitHub user name.

      .. parsed-literal::
         :class: highlight

         git remote add origin https\://github.com/*your_github_id*/fw-nrfconnect-nrf

      Finally, verify the remote repositories::

         git remote -v

      The output should look similar to this:

      .. parsed-literal::
         :class: highlight

         origin   https\://github.com/*your_github_id*/fw-nrfconnect-nrf (fetch)
         origin   https\://github.com/*your_github_id*/fw-nrfconnect-nrf (push)
         ncs      https\://github.com/NordicPlayground/fw-nrfconnect-nrf (fetch)
         ncs      https\://github.com/NordicPlayground/fw-nrfconnect-nrf (push)

#. Create a topic branch (based on the master branch) for your work.
   Replace *branch_name* with the name that you want to use for the local branch.

   .. parsed-literal::
      :class: highlight

      git checkout -b *branch_name* ncs/master


#. Make changes, test locally, change, test, test again, ...
   Check out the *sanitycheck* section in Zephyr's :ref:`zephyr:Contribution Tools` documentation as well.

#. When things look good, start the pull request process by adding your changed files.
   Replace *files* with the file paths of the files that you want to add (see the Git documentation for other options).

   .. parsed-literal::
      :class: highlight

      git add *files*

   You can see files that are not yet staged with the following command::

     git status

#. Verify that the changes to be committed look as you expect::

     git diff --cached

#. Commit the changes to your local repository::

     git commit -s

   The ``-s`` option automatically adds your ``Signed-off-by:`` to your commit message.
   This signature indicates that you agree with the :ref:`zephyr:DCO`.
   Without this line, your commit will be rejected.
   See the *Commit Guidelines* section in Zephyr's :ref:`zephyr:Contribution workflow` for specific guidelines for writing your commit messages.

#. Push the branch with your changes to your fork in your personal GitHub account.
   Replace *branch_name* with the name that you want to use for the branch in your fork.

   .. parsed-literal::
      :class: highlight

      git push -u origin *branch_name*

#. In your web browser, go to your forked repository or and click the :guilabel:`Compare & pull request` button for the branch for which you want to open a pull request.
   Alternatively, go to the URL that GitHub returns after the ``git push`` command to open the pull request.

#. Review the pull request changes, and verify that you are opening a pull request for the appropriate branch.
   The title and message from your commit message should appear as well.

#. Click on the :guilabel:`Submit` button, and your pull request is sent and ready for review.
   When review comments are made, you receive an email notification.
   Of course, you can always check your pull request on the `Pull requests tab`_.

#. While you are waiting for your pull request to be accepted and merged, you can create another branch to work on another issue.
   Be sure to base your new branch on master and not the previous branch.

   .. code-block::

      git checkout -b fix_another_issue ncs/master

   Then use the same process to work on this new topic branch.

#. If reviewers request changes to your patch, make changes to your local repository and push the new changes.
   First add the *files* again:

   .. parsed-literal::
      :class: highlight

      git add *files*

   Then udate the commit content::

      git commit --amend

   Finally, force-push your update:

   .. parsed-literal::
      :class: highlight

      git push --force origin *branch_name*

   Your original pull request will be updated with your changes.
   You do not need to resubmit the pull request.

#. If the automatic checks fail, you must make changes to your code to fix the issues and amend your commits as described above.
