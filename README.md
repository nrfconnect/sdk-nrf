# Hello

Welcome to my repo! It's a fork of the nRF Connect SDK. This `README.md`, the
GitHub workflows in `.github/`, the tests in `tests/`, and the documentation in
`docs/` have all been updated for this sake of this demo, but everything else is
the same.

This demo shows off the GitHub workflows, which will:

1. Build the source code itself
2. Run the unit tests
3. Archive the build artifacts for later retrieval
4. Build `docs/` as a static website
5. Publish the static website to this repo's GitHub Pages

## Unit Testing

Unit testing is done via `gtest`. Unit tests themselves will always be built as
part of the project, but running them requires targeting the `run_tests` target
in CMake:

``` shell
cmake --build build -t run_tests
```

## Build Artifacts

The build artifacts (including the HEX file) for each build will be published in
the workflow itself. Given a workflow ID `wid` and artifacts upload `aid`, the
artifacts can be downloaded at:

    https://github.com/joh06937/sdk-nrf/suites/`<wid>`/artifacts/`<aid>`

## Website Deployment

The GitHub Pages output can be found [here](https://joh06937.github.io/sdk-nrf).

## Triggers

The workflows are triggered off of pushes to the `main` branch and pull requests
being created and updated. Make one; I dare you ;-)

## Fun Oddities

I had a problem doing the GitHub Pages deployment in the same "job" as the
archiving of the build artifacts. Everything else worked fine together, so long
as I didn't archive the outputs. So, they're split into two "jobs" with the
documentation deployment depending on the build, since we don't want to publish
without knowing the build is good to go.

## Fun Facts

Fun fact: I'm in the upstream Zephyr and nRF Connect SDK Git histories! Ahh to
be so young again...
