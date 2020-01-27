//
// Copyright (c) 2018 Nordic Semiconductor ASA. All Rights Reserved.
//
// The information contained herein is confidential property of Nordic Semiconductor ASA.
// The use, copying, transfer or disclosure of such information is prohibited except by
// express written agreement with Nordic Semiconductor ASA.
//

@Library("CI_LIB") _

HashMap CI_STATE = lib_State.getConfig(JOB_NAME)
def TestExecutionList = [:]

properties([
  pipelineTriggers([
    parameterizedCron( [
        ((JOB_NAME =~ /latest\/night\/.*\/master/).find() ? CI_STATE.CFG.CRON.NIGHTLY : ''),
        ((JOB_NAME =~ /latest\/week\/.*\/master/).find() ? CI_STATE.CFG.CRON.WEEKLY : '')
    ].join('    \n') )
  ]),
  ( JOB_NAME.contains('sub/') ? disableResume() :  disableConcurrentBuilds() )
])

pipeline {

  parameters {
       booleanParam(name: 'RUN_DOWNSTREAM', description: 'if false skip downstream jobs', defaultValue: true)
       booleanParam(name: 'RUN_TESTS', description: 'if false skip testing', defaultValue: true)
       booleanParam(name: 'RUN_BUILD', description: 'if false skip building', defaultValue: true)
       string(name: 'PLATFORMS', description: 'Default Platforms to test', defaultValue: 'nrf9160_pca10090 nrf52_pca10040 nrf52840_pca10056 nrf5340_dk_nrf5340_cpuapp')
       string(name: 'jsonstr_CI_STATE', description: 'Default State if no upstream job', defaultValue: CI_STATE.CFG.INPUT_STATE_STR)
       choice(name: 'CRON', choices: ['COMMIT', 'NIGHTLY', 'WEEKLY'], description: 'Cron Test Phase')
  }
  agent { label CI_STATE.CFG.AGENT_LABELS }

  options {
    parallelsAlwaysFailFast()
    timeout(time: CI_STATE.CFG.TIMEOUT.time, unit: CI_STATE.CFG.TIMEOUT.unit)
  }

  environment {
      GH_TOKEN = credentials('nordicbuilder-compliance-token') // This token is used to by check_compliance to comment on PRs and use checks
      GH_USERNAME = "NordicBuilder"
      COMPLIANCE_ARGS = "-r NordicPlayground/fw-nrfconnect-nrf"
      ARCH = "-a arm"
      SANITYCHECK_OPTIONS_COMMON = '''--ninja \
                                      --board-root nrf/boards \
                                      --board-root zephyr/boards \
                                      --testcase-root nrf/samples \
                                      --testcase-root nrf/applications \
                                      --inline-logs --disable-unrecognized-section-test \
                                      --tag ci_build \
                                   '''
  }

  stages {
    stage('Load') { steps { script { CI_STATE = lib_State.load('NRF', CI_STATE) }}}
    stage('Specification') { steps { script {
      def TestStages = [:]
      TestStages["compliance"] = {
        node (CI_STATE.CFG.AGENT_LABELS) {
          stage('Compliance Test'){
            println "Using Node:$NODE_NAME"
            docker.image("$CI_STATE.CFG.DOCKER_REG/$CI_STATE.CFG.IMAGE_TAG").inside {
              dir('nrf') {
                checkout scm
                CI_STATE.SELF.REPORT_SHA = lib_Main.checkoutRepo(
                      CI_STATE.SELF.GIT_URL, "NRF", CI_STATE.SELF, false)
                lib_Status.set("PENDING", 'NRF', CI_STATE);
                lib_West.AddManifestUpdate("NRF", 'nrf',
                      CI_STATE.SELF.GIT_URL, CI_STATE.SELF.GIT_REF, CI_STATE)
              }
              lib_West.InitUpdate('nrf')
              lib_West.ApplyManifestUpdates(CI_STATE)

              dir('nrf') {
                script {
                  // If we're a pull request, compare the target branch against the current HEAD (the PR), and also report issues to the PR
                  def BUILD_TYPE = lib_Main.getBuildType(CI_STATE.SELF)
                  if (BUILD_TYPE == "PR") {
                    COMMIT_RANGE = "$CI_STATE.SELF.MERGE_BASE..$CI_STATE.SELF.REPORT_SHA"
                    COMPLIANCE_ARGS = "$COMPLIANCE_ARGS -p $CHANGE_ID -S $CI_STATE.SELF.REPORT_SHA -g"
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
                    sh """ \
                      (source ../zephyr/zephyr-env.sh && \
                      pip install --user -r ../tools/ci-tools/requirements.txt && \
                      ../tools/ci-tools/scripts/check_compliance.py $COMPLIANCE_ARGS --commits $COMMIT_RANGE) \
                    """
                  }
                  finally {
                    junit 'compliance.xml'
                    archiveArtifacts artifacts: 'compliance.xml'
                    lib_Main.storeArtifacts("compliance", 'compliance.xml', 'NRF', CI_STATE)
                  }
                }
              }
            }
            cleanWs()
          }
        }
      }

      if (CI_STATE.SELF.CRON == 'COMMIT') {
        println "Running Commit Tests"
      } else if (CI_STATE.SELF.CRON == 'NIGHTLY') {
        println "Running Nightly Tests"
      } else if (CI_STATE.SELF.CRON == 'WEEKLY') {
        println "Running Weekly Tests"
      }

      def PLATFORM_LIST = lib_Main.getPlatformList(CI_STATE.SELF.PLATFORMS)

      def COMPILER_LIST = ['gnuarmemb']  //'zephyr',
      def INPUT_MAP = [p : PLATFORM_LIST, c : COMPILER_LIST ]
      def PLATFORM_COMPILER_MAP = INPUT_MAP.values().combinations { args ->
          [INPUT_MAP.keySet().toList(), args].transpose().collectEntries { [(it[0]): it[1]]}
      }

      def sanityCheckStages = PLATFORM_COMPILER_MAP.collectEntries {
          ["SanityCheck\n${it.c}\n${it.p}" : generateParallelStage(it.p, it.c, JOB_NAME, CI_STATE, SANITYCHECK_OPTIONS_COMMON)]
      }

      if (CI_STATE.SELF.RUN_TESTS) {
          TestExecutionList['compliance'] = TestStages["compliance"]
      }

      if (CI_STATE.SELF.RUN_BUILD) {
        TestExecutionList = TestExecutionList.plus(sanityCheckStages)
      }

      println "TestExecutionList = $TestExecutionList"

    }}}

    stage('Exectuion') { steps { script {
      parallel TestExecutionList
    }}}

    stage('Trigger Downstream Jobs') {
      when { expression { CI_STATE.SELF.RUN_DOWNSTREAM } }
      steps { script { lib_Stage.runDownstream(JOB_NAME, CI_STATE) } }
    }

    stage('Report') {
      when { expression { CI_STATE.SELF.RUN_TESTS } }
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
      script { lib_Status.set("FAILURE", 'NRF', CI_STATE) }
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


def generateParallelStage(platform, compiler, JOB_NAME, CI_STATE, SANITYCHECK_OPTIONS_COMMON) {
  return {
    node (CI_STATE.CFG.AGENT_LABELS) {
      stage('.'){
        println "Using Node:$NODE_NAME"
        docker.image("$CI_STATE.CFG.DOCKER_REG/$CI_STATE.CFG.IMAGE_TAG").inside {
          dir('nrf') {
            checkout scm
            CI_STATE.SELF.REPORT_SHA = lib_Main.checkoutRepo(
                  CI_STATE.SELF.GIT_URL, "NRF", CI_STATE.SELF, false)
            lib_West.AddManifestUpdate("NRF", 'nrf',
                  CI_STATE.SELF.GIT_URL, CI_STATE.SELF.GIT_REF, CI_STATE)
          }
          lib_West.InitUpdate('nrf')
          lib_West.ApplyManifestUpdates(CI_STATE)
          PLATFORM_ARGS = lib_Main.getPlatformArgs(platform)
          SANITYCHECK_CMD = "./zephyr/scripts/sanitycheck $SANITYCHECK_OPTIONS_COMMON $PLATFORM_ARGS"
          FULL_SANITYCHECK_CMD = """
            export ZEPHYR_TOOLCHAIN_VARIANT='$compiler' && \
            source zephyr/zephyr-env.sh && \
            export && \
            $SANITYCHECK_CMD || \
            (sleep 10; $SANITYCHECK_CMD --only-failed --outdir=out-2nd-pass) || \
            (sleep 10; $SANITYCHECK_CMD --only-failed --outdir=out-3rd-pass)
          """
          println "FULL_SANITYCHECK_CMD = " + FULL_SANITYCHECK_CMD
          sh FULL_SANITYCHECK_CMD
        }
        cleanWs()
      }
    }
  }
}
