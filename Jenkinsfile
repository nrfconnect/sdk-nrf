// Due to JENKINS-42369 we put these defines outside the pipeline
def IMAGE_TAG = "ncs-toolchain:1.09"
def REPO_CI_TOOLS = "https://github.com/zephyrproject-rtos/ci-tools.git"
def REPO_CI_TOOLS_SHA = "bfe635f102271a4ad71c1a14824f9d5e64734e57"
def CI_CFG_URL = 'https://projecttools.nordicsemi.no/bitbucket/scm/ncs-test/test-ci-nrfconnect-cfg.git'
def CI_CFG_BRANCH = 'master'
def CI_CFG_OBJ = new HashMap()
def CI_STATE = new HashMap()

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

  parameters {
       string(name: 'RUN_DOWNSTREAM', description: 'if true skip actual testing', defaultValue: "true")
       string(name: 'RUN_TESTS', description: 'if true skip actual testing', defaultValue: "true")
       string(name: 'RUN_BUILD', description: 'if true skip actual testing', defaultValue: "true")
  }

  agent {
    docker {
      image "$IMAGE_TAG"
      label "docker && build-node && ncs && linux"
      args '-e PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/workdir/.local/bin'
    }
  }

  options {
    checkoutToSubdirectory('nrf')
    disableConcurrentBuilds()
  }

  triggers {
    cron(env.BRANCH_NAME == 'master' ? '0 */4 * * 1-7' : '') // Only master will be build periodically
  }

  environment {
      // ENVs for check-compliance
      GH_TOKEN = credentials('nordicbuilder-compliance-token') // This token is used to by check_compliance to comment on PRs and use checks
      GH_USERNAME = "NordicBuilder"
      COMPLIANCE_ARGS = "-r NordicPlayground/fw-nrfconnect-nrf"

      // Build all custom samples that match the ci_build tag
      SANITYCHECK_OPTIONS = "--board-root $WORKSPACE/nrf/boards --testcase-root $WORKSPACE/nrf/samples --testcase-root $WORKSPACE/nrf/applications --build-only --disable-unrecognized-section-test -t ci_build --inline-logs"
      ARCH = "-a arm"
      LC_ALL = "C.UTF-8"

      // ENVs for building (triggered by sanitycheck)
      ZEPHYR_TOOLCHAIN_VARIANT = 'gnuarmemb'
      GNUARMEMB_TOOLCHAIN_PATH = '/workdir/gcc-arm-none-eabi-7-2018-q2-update'
  }

  stages {
    stage('Load') {
      steps {
        println "Using Node:$NODE_NAME and Input Parameters:$params"
        dir("ci-tools") {
          git branch: "master", url: "$REPO_CI_TOOLS"
          sh "git checkout ${REPO_CI_TOOLS_SHA}"
        }
        dir("ci-cfg") {
          git branch: CI_CFG_BRANCH, url: CI_CFG_URL, credentialsId: '0075d160-104a-4b0f-b21e-0a66c6d526df'
          script {
            def CI_CFG_SHA = sh (script: "git rev-parse HEAD | tr -d '\\n' ", returnStdout: true)
            println "CI_CFG_SHA = ${CI_CFG_SHA}"
            CI_CFG_OBJ.load = load "${pwd()}/load.gvy"
            CI_CFG_OBJ << CI_CFG_OBJ.load.all()
            // println "CI_CFG_OBJ = $CI_CFG_OBJ"
            CI_STATE = CI_CFG_OBJ.state.store('NRF', CI_STATE)
            println "CI_STATE = $CI_STATE"
          }
        }
      }
    }
    stage('Checkout repositories') {
      when { expression { CI_STATE.NRF.RUN_TESTS.toBoolean() } }
      steps {
        script {
          CI_STATE.NRF.REPORT_SHA = CI_CFG_OBJ.main.checkoutRepo(CI_STATE.NRF.GIT_URL, "nrf", CI_STATE, false)
          println "CI_STATE.NRF.REPORT_SHA = " + CI_STATE.NRF.REPORT_SHA
          CI_CFG_OBJ.main.westInitUpdate('nrf')
        }
      }
    }
    stage('Run compliance check') {
      when { expression { CI_STATE.NRF.RUN_TESTS.toBoolean() } }
      steps {
        dir('nrf') {
          script {
            // If we're a pull request, compare the target branch against the current HEAD (the PR), and also report issues to the PR
            def BUILD_TYPE = CI_CFG_OBJ.main.getBuildType(CI_STATE)
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
            }
          }
        }
      }
    }
    stage('Build samples') {
      when { expression { CI_STATE.NRF.RUN_BUILD.toBoolean() } }
      steps {
        // Create a folder to store artifacts in
        sh 'mkdir artifacts'

        // Build all the samples
        dir('zephyr') {
          sh "source zephyr-env.sh && ./scripts/sanitycheck $SANITYCHECK_OPTIONS"
        }

        script {
          /* Rename the nrf52 desktop samples */
          desktop_platforms = ['nrf52840_pca20041', 'nrf52_pca20037', 'nrf52840_pca10059']
          for(int i=0; i<desktop_platforms.size(); i++) {
            file_path = "zephyr/sanity-out/${desktop_platforms[i]}/nrf_desktop/test/zephyr/zephyr.hex"
            check_and_store_sample("$file_path", "nrf_desktop_${desktop_platforms[i]}.hex")
            file_path = "zephyr/sanity-out/${desktop_platforms[i]}/nrf_desktop/test_zrelease/zephyr/zephyr.hex"
            check_and_store_sample("$file_path", "nrf_desktop_${desktop_platforms[i]}_ZRelease.hex")
          }

          /* Rename the nrf9160 samples */
          samples = ['spm']
          for(int i=0; i<samples.size(); i++)
          {
            file_path = "zephyr/sanity-out/nrf9160_pca10090/nrf9160/${samples[i]}/test_build/zephyr/zephyr.hex"
            check_and_store_sample("$file_path", "${samples[i]}_nrf9160_pca10090.hex")
          }
          ns_samples = ['lte_ble_gateway', 'at_client']
          for(int i=0; i<ns_samples.size(); i++)
          {
            file_path = "zephyr/sanity-out/nrf9160_pca10090ns/nrf9160/${ns_samples[i]}/test_build/zephyr/zephyr.hex"
            check_and_store_sample("$file_path", "${ns_samples[i]}_nrf9160_pca10090ns.hex")
          }
          ns_apps = ['asset_tracker']
          for(int i=0; i<ns_apps.size(); i++)
          {
            file_path = "zephyr/sanity-out/nrf9160_pca10090ns/${ns_apps[i]}/test_build/zephyr/zephyr.hex"
            check_and_store_sample("$file_path", "${ns_apps[i]}_nrf9160_pca10090ns.hex")
          }
        }
        archiveArtifacts allowEmptyArchive: true, artifacts: 'artifacts/*.hex'
      }
    }
    stage('Trigger testing build') {
      when { expression { CI_STATE.NRF.RUN_DOWNSTREAM.toBoolean() } }
      steps {
        script {
          CI_STATE.NRF.WAITING = true
          def jsonstr_CI_STATE = CI_CFG_OBJ.state.HashMap2Str(CI_STATE)
          println "jsonstr_CI_STATE = " + jsonstr_CI_STATE
          def DOWNSTREAM_PROJECTS = CI_CFG_OBJ.main.getDownStreamProjects(CI_STATE, 'NRF')
          println "DOWNSTREAM_PROJECTS = " + DOWNSTREAM_PROJECTS
          def projs = [:]
          DOWNSTREAM_PROJECTS.each {
            projs["${it}"] = {
              build job: "${it}", propagate: true, wait: true, parameters: [
                        string(name: 'jsonstr_CI_STATE', value: CI_CFG_OBJ.state.HashMap2Str(CI_STATE))]
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
