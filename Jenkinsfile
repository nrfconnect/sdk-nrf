//
// Copyright (c) 2018 Nordic Semiconductor ASA. All Rights Reserved.
//
// The information contained herein is confidential property of Nordic Semiconductor ASA.
// The use, copying, transfer or disclosure of such information is prohibited except by
// express written agreement with Nordic Semiconductor ASA.
//

@Library("CI_LIB") _
HashMap CI_STATE = lib_State.getConfig(JOB_NAME)
properties(lib_State.getTriggers())

def TestExecutionList = [:]

pipeline {

  parameters {
       booleanParam(name: 'RUN_DOWNSTREAM', description: 'if false skip downstream jobs', defaultValue: true)
       booleanParam(name: 'RUN_TESTS', description: 'if false skip testing', defaultValue: true)
       booleanParam(name: 'RUN_BUILD', description: 'if false skip building', defaultValue: true)
       string(name: 'PLATFORMS', description: 'Default Platforms to test', defaultValue: 'nrf9160dk_nrf9160 nrf52dk_nrf52832 nrf52840dk_nrf52840 nrf5340pdk_nrf5340_cpuapp')
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
      COMPLIANCE_ARGS = "-r nrfconnect/sdk-nrf"
      ARCH = "-a arm"
      SANITYCHECK_OPTIONS_COMMON = '''--ninja \
                                      --board-root nrf/boards \
                                      --board-root zephyr/boards \
                                      --testcase-root nrf/samples \
                                      --testcase-root nrf/applications \
                                      --testcase-root nrf/tests \
                                      --inline-logs --disable-unrecognized-section-test \
                                      --tag ci_build \
                                      --retry-failed 7 \
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
                    COMPLIANCE_ARGS = "$COMPLIANCE_ARGS $CI_STATE.SELF.CUSTOM_COMPLIANCE_ARGS -p $CHANGE_ID -S $CI_STATE.SELF.REPORT_SHA -g"
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
                      pip install --user pylint && \
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

    stage('Execution') { steps { script {
      parallel TestExecutionList
      lib_Status.set("${currentBuild.currentResult}",  'NRF', CI_STATE)
    }}}

    stage('Trigger Downstream Jobs') {
      when { expression { CI_STATE.SELF.RUN_DOWNSTREAM } }
      steps { script { runDownstreamCustom(JOB_NAME, CI_STATE) } }
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
    always {   script { echo "always"; lib_Status.set( "${currentBuild.currentResult}" , 'FULL_CI', CI_STATE) } }

    /* uncomment if logic is needed
    success  { }
    aborted  { }
    unstable { }
    failure  { }
    */

    cleanup  { script { echo "cleanup"; cleanWs disableDeferredWipeout: true, deleteDirs: true } }
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
            pip install --user -r nrf/scripts/requirements-ci.txt && \
            export && \
            $SANITYCHECK_CMD
          """
          println "FULL_SANITYCHECK_CMD = " + FULL_SANITYCHECK_CMD
          sh FULL_SANITYCHECK_CMD
        }
        cleanWs()
      }
    }
  }
}

def runDownstreamCustom(String JOB_NAME, HashMap CI_STATE) {
  def tmp_stage = {
    CI_STATE.SELF.STARTED_DOWNSTREAM = true

    def jobs = [:]

	def DOWNSTREAM_JOBS = ["latest/sub/test-fw-nrfconnect-nrf/v1.3-branch:[propagate:true, wait:true]", "latest/sub/test-fw-nrfconnect-nrf_doc/v1.3-branch:[propagate:true, wait:true]"]
    println "DOWNSTREAM_JOBS = " + DOWNSTREAM_JOBS
    DOWNSTREAM_JOBS.each { job ->
      println "Starting JOB = $job"

      CI_STATE.SELF.remove('jsonstr_CI_STATE')   // json string to pass is size limited
      CI_STATE.ORIGIN.remove('jsonstr_CI_STATE')
      // println 'GREP_KEY: CI_STATE =\n' + lib_util.prettyPrintMap(CI_STATE) + '\n'

      jobs["$job.key"] = {
        def lib_util = new lib_Util()
        CI_STATE_COPY = CI_STATE.clone()

        job_branch = job.key.split('/')[-1]
        def on_commit_latest_result = "SUCCESS"
        if ( job_branch == 'master' ) {
          on_commit_latest_result = lib_util.getLatestBuildStatus(job.key)
        }

        if ( (job.value.propagate == false) && !JOB_NAME.contains('latest/') ) { println "SKIPPING: $job.key"; return }

        CI_STATE_COPY.HANDOFF = new HashMap()



        if (job.value.propagate == false) {

          println "STARTED: $job.key (Not waiting for result)"
          CI_STATE_COPY.HANDOFF.PROPAGATING = false

        } else if ( ( job.value.propagate == true ) &&
                    ( JOB_NAME.contains('/test-') ) &&
                    ( !JOB_NAME.contains('/week/') ) &&
                    ( !JOB_NAME.contains('/night/') ) &&
                    ( !['SUCCESS'].contains(on_commit_latest_result) )
                  ) {

          println "IGNORING RESULT: because $job.key/master is failing on the commit trigger."
          job.value.propagate = false
          CI_STATE_COPY.HANDOFF.PROPAGATING = false

        } else {

          println "PROPAGATING RESULT: $job.key"
          CI_STATE_COPY.HANDOFF.PROPAGATING = true

        }

        def build_failed_to_execute = false
        try {
          result = build job: job.key, propagate: false, wait: job.value.propagate,
                             parameters: [ string(name: 'jsonstr_CI_STATE',
                                                  value: lib_util.HashMap2Str(CI_STATE_COPY))]
        } catch (Exception e) {
          build_failed_to_execute = true
        } finally {
          if (build_failed_to_execute) {
            catchError(stageResult: 'FAILURE', buildResult: 'FAILURE') {
              error("Downstream Job:$job.key failed to run.")
            }
            return
          }

          if (job.value.propagate) {
            def SUB_JOB_URL = result.getAbsoluteUrl()
            println 'URL: ' + SUB_JOB_URL
            println "RESULT: $job.key == $result.result"

            catchError(stageResult: 'UNSTABLE', buildResult: 'UNSTABLE') {
              if (result.result == 'UNSTABLE') {
                error("Downstream Job:$job.key result=" + result.result)
              }
            }
            catchError(stageResult: 'FAILURE', buildResult: 'FAILURE') {
              if ( job.value.propagate && (result.result == 'FAILURE') ) {
                error("Downstream Job:$job.key result=" + result.result)
              }
            }
          } else {
            println "TRIGGERED: $job.key (Not waiting for result)"
            catchError(stageResult: 'UNSTABLE', buildResult: 'SUCCESS') {
              error("Downstream Job:$job.key result is not propagated. You can ignore this job.")
            }
          }
        }
      }
    }
    parallel jobs
  }
  tmp_stage()
}
