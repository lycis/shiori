*** Settings ***
Resource         resources/shiori.resource
Test Setup       Create Isolated Test Workspace
Test Teardown    Remove Isolated Test Workspace

*** Test Cases ***
Init Creates Local Configuration
    ${result}=    Run Shiori    init
    Shiori Should Succeed    ${result}
    File Should Exist    ${TEST_CWD}${/}.shiori
    ${config}=    Get File    ${TEST_CWD}${/}.shiori    encoding=UTF-8
    Should Contain    ${config}    version: 1
    Should Contain    ${config}    base_dir: ${TEST_CWD}
    Should Contain    ${config}    color: true

Init Refuses To Overwrite Configuration
    ${first}=     Run Shiori    init
    Shiori Should Succeed    ${first}
    ${before}=    Get File    ${TEST_CWD}${/}.shiori    encoding=UTF-8

    ${second}=    Run Shiori    init
    Shiori Should Fail    ${second}
    Combined Output Should Contain    ${second}    already exists
    ${after}=    Get File    ${TEST_CWD}${/}.shiori    encoding=UTF-8
    Should Be Equal    ${after}    ${before}

Reinit Replaces Existing Configuration
    Create File    ${TEST_CWD}${/}.shiori    invalid configuration
    ${result}=    Run Shiori    init    --reinit
    Shiori Should Succeed    ${result}
    ${config}=    Get File    ${TEST_CWD}${/}.shiori    encoding=UTF-8
    Should Contain        ${config}    version: 1
    Should Not Contain    ${config}    invalid configuration

Local Configuration Takes Precedence Over Home Configuration
    Create File
    ...    ${TEST_HOME}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_HOME}${\n}
    Write Shiori Config
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    base_dir: ${TEST_DATA}

Home Configuration Is Used As Fallback
    Create File
    ...    ${TEST_HOME}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_DATA}${\n}
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    base_dir: ${TEST_DATA}

Missing Color Setting Defaults To True
    Write Shiori Config
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    color: true

Color Setting Accepts False
    Write Shiori Config    color=false
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    color: false

Invalid Color Setting Is Rejected
    Write Shiori Config    color=sometimes
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    color must be true or false
