
@Library("CI_LIB") _

def AGENT_LABELS = lib_Main.getAgentLabels(JOB_NAME)
def IMAGE_TAG    = lib_Main.getDockerImage(JOB_NAME)
def TIMEOUT      = lib_Main.getTimeout(JOB_NAME)
def INPUT_STATE  = lib_Main.getInputState(JOB_NAME)
def CI_STATE     = new HashMap()

pipeline {

  parameters {
       booleanParam(name: 'RUN_DOWNSTREAM', description: 'if false skip downstream jobs', defaultValue: true)
       booleanParam(name: 'RUN_TESTS', description: 'if false skip testing', defaultValue: true)
       booleanParam(name: 'RUN_BUILD', description: 'if false skip building', defaultValue: true)
       string(name: 'PLATFORMS', description: 'Default Platforms to test', defaultValue: 'nrf9160_pca10090 nrf52_pca10040 nrf52840_pca10056')
       string(name: 'jsonstr_CI_STATE', description: 'Default State if no upstream job', defaultValue: INPUT_STATE)
  }

  agent {
    docker {
      image IMAGE_TAG
      label AGENT_LABELS
    }
  }

  options {
    checkoutToSubdirectory('nrf')
    parallelsAlwaysFailFast()
    timeout(time: TIMEOUT.time, unit: TIMEOUT.unit)
  }

  triggers {
    cron(env.BRANCH_NAME == 'master' ? '0 */12 * * 1-7' : '') // Only master will be build periodically
  }

  environment {
      GH_TOKEN = credentials('nordicbuilder-compliance-token') // This token is used to by check_compliance to comment on PRs and use checks
      GH_USERNAME = "NordicBuilder"
      COMPLIANCE_ARGS = "-r NordicPlayground/fw-nrfconnect-nrf"
      ARCH = "-a arm"
      ZEPHYR_TOOLCHAIN_VARIANT = 'gnuarmemb'
      GNUARMEMB_TOOLCHAIN_PATH = '/workdir/gcc-arm-none-eabi-7-2018-q2-update'

      SANITYCHECK_OPTIONS_COMMON = '''--ninja \
                                      --board-root nrf/boards \
                                      --board-root zephyr/boards \
                                      --testcase-root nrf/samples \
                                      --testcase-root nrf/applications \
                                      --inline-logs --disable-unrecognized-section-test \
                                   '''
  }

  stages {
    stage('Load') { steps { script { CI_STATE = lib_Stage.load('NRF') }}
    }
    stage('Checkout') {
      steps { script {
        lib_Main.cloneCItools(JOB_NAME)
        dir('nrf') {
          CI_STATE.NRF.REPORT_SHA = lib_Main.checkoutRepo(CI_STATE.NRF.GIT_URL, "NRF", CI_STATE.NRF, false)
          lib_West.AddManifestUpdate("NRF", 'nrf', CI_STATE.NRF.GIT_URL, CI_STATE.NRF.GIT_REF, CI_STATE)
        }
      }}
    }
    stage('Get nRF && Apply Parent Manifest Updates') {
      when { expression { CI_STATE.NRF.RUN_TESTS || CI_STATE.NRF.RUN_BUILD } }
      steps { script {
        lib_Status.set("PENDING", 'NRF', CI_STATE)
        lib_West.InitUpdate('nrf')
        lib_West.ApplyManifestUpdates(CI_STATE)
      }}
    }
    stage('Run compliance check') {
      when { expression { CI_STATE.NRF.RUN_TESTS } }
      steps {
        dir('nrf') {
          script {
            // If we're a pull request, compare the target branch against the current HEAD (the PR), and also report issues to the PR
            def BUILD_TYPE = lib_Main.getBuildType(CI_STATE.NRF)
            if (BUILD_TYPE == "PR") {
              COMMIT_RANGE = "$CI_STATE.NRF.MERGE_BASE..$CI_STATE.NRF.REPORT_SHA"
              COMPLIANCE_ARGS = "$COMPLIANCE_ARGS -p $CHANGE_ID -S $CI_STATE.NRF.REPORT_SHA -g"
              println "Building a PR [$CHANGE_ID]: $COMMIT_RANGE"
            }
            else if (BUILD_TYPE == "TAG") {
              COMMIT_RANGE = "tags/${env.BRANCH_NAME}..tags/${env.BRANCH_NAME}"
              println "Building a Tag: " + COMMIT_RANGE
            }
            // If not a PR, it's a non-PR-branch or master build. Compare against the origin.
            else if (BUILD_TYPE == "BRANCH") {
              COMMIT_RANGE = "origin/${env.BRANCH_NAME}..HEAD"
              println "Building a Branch: " + COMMIT_RANGE
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
              lib_Main.storeArtifacts("compliance", 'compliance.xml', 'NRF', CI_STATE)
            }
          }
        }
      }
    }
    stage('Run Sanity Check') {
      when { expression { CI_STATE.NRF.RUN_BUILD } }
      steps { script {
        SANITYCHECK_OPTIONS = """ $SANITYCHECK_OPTIONS_COMMON \
                                  --tag ci_build \
                              """
        PLATFORM_ARGS = lib_Main.getPlatformArgs(CI_STATE.NRF.PLATFORMS)
        SANITYCHECK_CMD = "./zephyr/scripts/sanitycheck $SANITYCHECK_OPTIONS $PLATFORM_ARGS"
        FULL_SANITYCHECK_CMD = """
          source zephyr/zephyr-env.sh && \
          $SANITYCHECK_CMD || \
          (sleep 10; $SANITYCHECK_CMD --only-failed --outdir=out-2nd-pass) || \
          (sleep 10; $SANITYCHECK_CMD --only-failed --outdir=out-3rd-pass)
        """
        println "FULL_SANITYCHECK_CMD = " + FULL_SANITYCHECK_CMD
        sh FULL_SANITYCHECK_CMD
      } }
    }
    stage('Trigger Downstream Jobs') {
      when { expression { CI_STATE.NRF.RUN_DOWNSTREAM } }
      steps { script {
          CI_STATE.NRF.WAITING = true
          def DOWNSTREAM_JOBS = lib_Main.getDownStreamJobs(JOB_NAME)
          println 'DOWNSTREAM_JOBS: ' + DOWNSTREAM_JOBS
          def jobs = [:]
          DOWNSTREAM_JOBS.each {
            jobs["${it}"] = {
              build job: "${it}", propagate: CI_STATE.NRF.WAITING, wait: CI_STATE.NRF.WAITING,
                  parameters: [string(name: 'jsonstr_CI_STATE', value: lib_Util.HashMap2Str(CI_STATE))]
            }
          }
          parallel jobs
      } }
    }
    stage('Report') {
      when { expression { CI_STATE.NRF.RUN_TESTS } }
      steps { script {
          println 'no report generation yet'
      } }
    }
  }
  post {
    // This is the order that the methods are run. {always->success/abort/failure/unstable->cleanup}
    always {
      echo "always"
    }
    success {
      echo "success"
      script { lib_Status.set("SUCCESS", 'NRF', CI_STATE) }
    }
    aborted {
      echo "aborted"
      script { lib_Status.set("ABORTED", 'NRF', CI_STATE) }
    }
    unstable {
      echo "unstable"
    }
    failure {
      echo "failure"
      script { lib_Status.set("FAILURE", 'NRF', CI_STATE) }
    }
    cleanup {
        echo "cleanup"
        cleanWs()
    }
  }
}
