// Due to JENKINS-42369 we put these defines outside the pipeline
def IMAGE_TAG = "ncs-toolchain:1.06"
def REPO_ZEPHYR = "https://github.com/NordicPlayground/fw-nrfconnect-zephyr.git"
def REPO_NRFXLIB = "https://github.com/NordicPlayground/nrfxlib.git"

// Function to get the current repo URL, to be propagated to the downstream job
def getRepoURL() {
  dir('nrf') {
    sh "git config --get remote.origin.url > .git/remote-url"
    return readFile(".git/remote-url").trim()
  }
}

pipeline {
  agent {
    docker {
      image "$IMAGE_TAG"
      label "docker && ncs"
    }
  }
  options {
    // Checkout the repository to this folder instead of root
    checkoutToSubdirectory('nrf')
  }

  environment {
      // Build all custom samples that match the ci_build tag
      SANITYCHECK_OPTIONS = "--board-root $WORKSPACE/nrf/boards --testcase-root $WORKSPACE/nrf/samples --build-only --disable-unrecognized-section-test -t ci_build --inline-logs"
      ARCH = "-a arm"
      LC_ALL = "C.UTF-8"
      DOWNSTREAM_PROJECTS = credentials('fw-nrfconnect-nrf-jobs')
  }

  stages {
    stage('Checkout repositories') {
      steps {
        dir("zephyr") {
          git branch: "master", url: "$REPO_ZEPHYR", credentialsId: 'github'
        }
        dir("nrfxlib") {
          git branch: "master", url: "$REPO_NRFXLIB", credentialsId: 'github'
        }
      }
    }

    stage('Testing') {
      parallel {
        stage('Build samples') {
          steps {
            // Create a folder to store artifacts in
            sh 'mkdir artifacts'

            // Build all the samples
            dir('zephyr') {
              sh "source zephyr-env.sh && ./scripts/sanitycheck $SANITYCHECK_OPTIONS"
            }

            // Export artifacts needed for downstream projects.
            // Platform array assumed to be <chip>_<board> and artifact will be named <board>.hex
            script {
              desktop_platforms = ['nrf52840_pca20041', 'nrf52840_pca10056', 'nrf52_pca63519']
              for(int i=0; i<desktop_platforms.size(); i++) {
                file_path = "zephyr/sanity-out/${desktop_platforms[i]}/nrf_desktop/test/zephyr/zephyr.hex"
                if (fileExists(file_path)) {
                  sh "cp ${file_path} artifacts/${desktop_platforms[i].split('_')[1]}.hex"
                }
                else {
                  echo "Build for ${desktop_platforms[i]} failed, or no output was found"
                  sh "cat zephyr/sanity-out/${desktop_platforms[i]}/nrf_desktop/test/build.log"
                }
              }
            }

            // Check if the files were successfully built
            script {
              if (fileExists('artifacts/pca20041.hex') && \
                  fileExists('artifacts/pca10056.hex') && \
                  fileExists('artifacts/pca63519.hex')) {
                echo "nrf_desktop build successful!"
                archiveArtifacts artifacts: 'artifacts/*.hex'
              }
              else {
                echo "nrf_desktop build failed!"
                currentBuild.result = 'FAILURE'
              }
            }
          }
        }

        stage('Run compliance check') {
          steps {
            // Define a Groovy script block, which allows things like try/catch and if/else. If not, the junit command will not be run if check-compliance fails
            script {
              // If we're a pull request, compare the target branch against the current HEAD (the PR)
              if (env.CHANGE_TARGET) {
                COMMIT_RANGE = "origin/${env.CHANGE_TARGET}..HEAD"
              }
              // If not a PR, it's a non-PR-branch or master build. Compare against the origin.
              else {
                COMMIT_RANGE = "origin/${env.BRANCH_NAME}..HEAD"
              }
              // Run the compliance check
              try {
                sh "(source zephyr/zephyr-env.sh && cd nrf && ../zephyr/scripts/ci/check-compliance.py --commits $COMMIT_RANGE)"
              }
              finally {
                junit 'nrf/compliance.xml'
              }
            }
          }
        }
      }
    }

    stage('Trigger testing build') {
      steps {
        script {
          def projs = [:]
          env.DOWNSTREAM_PROJECTS.split(',').each {
            projs["${it}"] = {
              build job: "${it}", propagate: true, wait: false, parameters: [string(name: 'branchname', value: "$BRANCH_NAME"),
                                                                             string(name: 'API_URL', value: "${getRepoURL()}"),
                                                                             string(name: 'API_COMMIT', value: "$GIT_COMMIT"),
                                                                             string(name: 'API_PR_NAME', value: "$CHANGE_TITLE")]
            }
          }
          parallel projs
        }
      }
    }
  }

  post {
    always {
      // Clean up the working space at the end (including tracked files)
      cleanWs()
    }
  }
}
