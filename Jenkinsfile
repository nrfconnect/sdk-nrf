// Due to JENKINS-42369 we put these defines outside the pipeline
def IMAGE_TAG = "ncs-toolchain:1.04"
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
      SANITYCHECK_OPTIONS = " --inline-logs"
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
        stage('Build nrf_desktop') {
          steps {
            // Use paranthesis to avoid actually changing the current working directory
            sh "mkdir nrf/samples/nrf_desktop/build_pca20041 nrf/samples/nrf_desktop/build_pca10056 nrf/samples/nrf_desktop/build_pca63519"
            sh "(source zephyr/zephyr-env.sh && cd nrf/samples/nrf_desktop/build_pca20041 && cmake .. -DBOARD=nrf52840_pca20041 && make -j 8)"
            sh "(source zephyr/zephyr-env.sh && cd nrf/samples/nrf_desktop/build_pca10056 && cmake .. -DBOARD=nrf52840_pca10056 && make -j 8)"
            sh "(source zephyr/zephyr-env.sh && cd nrf/samples/nrf_desktop/build_pca63519 && cmake .. -DBOARD=nrf52_pca63519    && make -j 8)"

            // Check if the files were successfully built
            script {
              if (fileExists('nrf/samples/nrf_desktop/build_pca20041/zephyr/zephyr.hex') && \
                  fileExists('nrf/samples/nrf_desktop/build_pca10056/zephyr/zephyr.hex') && \
                  fileExists('nrf/samples/nrf_desktop/build_pca63519/zephyr/zephyr.hex')) {
                echo "nrf_desktop build successful!"
                sh "cp 'nrf/samples/nrf_desktop/build_pca20041/zephyr/zephyr.hex' pca20041.hex"
                sh "cp 'nrf/samples/nrf_desktop/build_pca10056/zephyr/zephyr.hex' pca10056.hex"
                sh "cp 'nrf/samples/nrf_desktop/build_pca63519/zephyr/zephyr.hex' pca63519.hex"
                archiveArtifacts artifacts: '*.hex'
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
              build job: "${it}", propagate: true, wait: false, parameters: [string(name: 'branchname', value: "$BRANCH_NAME"), string(name: 'API_URL', value: "${getRepoURL()}"), string(name: 'API_COMMIT', value: "$GIT_COMMIT")]
            }
          }
          parallel projs
        }
      }
    }
  }

  // post {
    // always {
      // // Clean up the working space at the end (including tracked files)
      // cleanWs()
    // }
  // }
}
