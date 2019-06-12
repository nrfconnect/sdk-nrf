// Due to JENKINS-42369 we put these defines outside the pipeline
def IMAGE_TAG = "ncs-toolchain:1.09"
def REPO_CI_TOOLS = "https://github.com/zephyrproject-rtos/ci-tools.git"
def REPO_CI_TOOLS_SHA = "bfe635f102271a4ad71c1a14824f9d5e64734e57"

// Function to get the current repo URL, to be propagated to the downstream job
def getRepoURL() {
  dir('nrf') {
    sh "git config --get remote.origin.url > .git/remote-url"
    return readFile(".git/remote-url").trim()
  }
}


def getPRHEADSHA() {
  dir('nrf') {
    sh "git fetch $GIT_URL refs/pull/\$(echo -n $GIT_BRANCH | sed 's/PR-//')/head"
    def GH_PR_HEAD_SHA = sh (script: "git rev-parse FETCH_HEAD | tr -d '\\n' ", returnStdout: true)
    return "$GH_PR_HEAD_SHA"
  }
}

void setBuildStatus(String message, String state) {
  step([
      $class: "GitHubCommitStatusSetter",
      reposSource: [$class: "ManuallyEnteredRepositorySource", url: "$GIT_URL"],
      contextSource: [$class: "ManuallyEnteredCommitContextSource", context: "continuous-integration/jenkins/nrf-ci"],
      commitShaSource: [$class: "ManuallyEnteredShaSource", sha: "$GIT_COMMIT"],
      errorHandlers: [[$class: "ChangingBuildStatusErrorHandler", result: "UNSTABLE"]],
      statusResultSource: [ $class: "ConditionalStatusResultSource", results: [[$class: "AnyBuildResult", message: message, state: state]] ]
  ]);
}

def check_and_store_sample(path, new_name) {
  script {
    if (fileExists(file_path)) {
      sh "cp ${path} artifacts/${new_name}"
    }
    else {
      echo "Build for ${new_name} failed"
      currentBuild.result = 'FAILURE'
    }
  }
}

pipeline {
  agent {
    docker {
      image "$IMAGE_TAG"
      label "docker && build-node && ncs && linux && node-build-02"
      args '-e PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/workdir/.local/bin'
    }
  }
  options {
    // Checkout the repository to this folder instead of root
    checkoutToSubdirectory('nrf')
  }

  environment {
      // ENVs for check-compliance
      GH_TOKEN = credentials('nordicbuilder-compliance-token') // This token is used to by check_compliance to comment on PRs and use checks
      GH_USERNAME = "NordicBuilder"
      COMPLIANCE_ARGS = "-r NordicPlayground/fw-nrfconnect-nrf"
      COMPLIANCE_REPORT_ARGS = "-p $CHANGE_ID -S ${getPRHEADSHA()} -g"

      // Build all custom samples that match the ci_build tag
      SANITYCHECK_OPTIONS = "--board-root $WORKSPACE/nrf/boards --testcase-root $WORKSPACE/nrf/samples --testcase-root $WORKSPACE/nrf/applications --build-only --disable-unrecognized-section-test -t ci_build --inline-logs"
      ARCH = "-a arm"
      LC_ALL = "C.UTF-8"

      // ENVs for building (triggered by sanitycheck)
      ZEPHYR_TOOLCHAIN_VARIANT = 'gnuarmemb'
      GNUARMEMB_TOOLCHAIN_PATH = '/workdir/gcc-arm-none-eabi-7-2018-q2-update'

      // Projects to trigger after this one is built
      DOWNSTREAM_PROJECTS = credentials('fw-nrfconnect-nrf-jobs')
  }

  stages {
    stage('Checkout repositories') {
      steps {
        // Fetch the tools used to checking compliance
        dir("ci-tools") {
          git branch: "master", url: "$REPO_CI_TOOLS"
          sh "git checkout ${REPO_CI_TOOLS_SHA}"
        }
        // Initialize west
        sh "west init -l nrf/"

        // Checkout
        sh "west update"
      }
    }
    stage('Run compliance check') {
      steps {
        // Define a Groovy script block, which allows things like try/catch and if/else. If not, the junit command will not be run if check-compliance fails
        dir('nrf') {
          script {
            // If we're a pull request, compare the target branch against the current HEAD (the PR), and also report issues to the PR
            println "CHANGE_TARGET = ${env.CHANGE_TARGET}"
            println "BRANCH_NAME = ${env.BRANCH_NAME}"
            println "TAG_NAME = ${env.TAG_NAME}"
            println("NODE_NAME = ${NODE_NAME}")
            println("GIT_COMMIT = ${GIT_COMMIT}")
            println("GIT_BRANCH = ${GIT_BRANCH}")
            println("GIT_LOCAL_BRANCH = ${GIT_LOCAL_BRANCH}")
            println("GIT_URL = ${GIT_URL}")
            println("getPRHEADSHA = ${getPRHEADSHA()} there is text after this")

            sh "(git remote --verbose)"

            if (env.CHANGE_TARGET) {
              COMMIT_RANGE = "origin/${env.CHANGE_TARGET}..HEAD"
              COMPLIANCE_ARGS = "$COMPLIANCE_ARGS $COMPLIANCE_REPORT_ARGS"
              println "Building a PR: ${COMMIT_RANGE}"
            }
            else if (env.TAG_NAME) {
              COMMIT_RANGE = "tags/${env.BRANCH_NAME}..tags/${env.BRANCH_NAME}"
              println "Building a Tag: ${COMMIT_RANGE}"
            }
            // If not a PR, it's a non-PR-branch or master build. Compare against the origin.
            else if (env.BRANCH_NAME) {
              COMMIT_RANGE = "origin/${env.BRANCH_NAME}..HEAD"
              println "Building a Branch: ${COMMIT_RANGE}"
            }
            else {
                assert condition : "Build fails because it is not a PR/Tag/Branch"
            }

            // Run the compliance check
            try {
              sh "(source ../zephyr/zephyr-env.sh && ../ci-tools/scripts/check_compliance.py $COMPLIANCE_ARGS --commits $COMMIT_RANGE)"
            }
            finally {
              junit 'compliance.xml'
              archiveArtifacts artifacts: 'compliance.xml'
            }
          }
        }
      }
    }
    stage('Build samples') {
      steps {
        // Create a folder to store artifacts in
        sh 'mkdir artifacts'
      }
    }
  }

  post {
    always {
      // Clean up the working space at the end (including tracked files)
      println("Skip cleanup")
      //cleanWs()
    }
    success {
        setBuildStatus("Nrf CI passed", "SUCCESS");
    }
    aborted {
        setBuildStatus("Nrf CI aborted", "ERROR");
    }
    failure {
        setBuildStatus("Nrf CI failed, see link to downstream job in main CI", "FAILURE");
    }
  }
}
