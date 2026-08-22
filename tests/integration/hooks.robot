*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Hook Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Hook Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory
    ${platform}=    Evaluate    platform.system()    modules=platform
    Skip If    '${platform}' != 'Windows'    Hook execution is currently implemented only on Windows.

*** Test Cases ***
Successful Command Invokes Hook With Environment
    ${hook}=    Catenate    SEPARATOR=\n
    ...    @echo off
    ...    > hook-result.txt echo version=%SHIORI_VERSION%
    ...    >> hook-result.txt echo command=%SHIORI_COMMAND%
    ...    >> hook-result.txt echo args=%SHIORI_COMMAND_ARGS%
    Create File    ${TEST_DATA}${/}record-hook.cmd    ${hook}${\n}    encoding=UTF-8
    Write Shiori Config    hook=record-hook.cmd

    ${result}=    Run Shiori    add    Hook argument with spaces
    Shiori Should Succeed    ${result}
    File Should Exist    ${TEST_DATA}${/}hook-result.txt
    ${hook_result}=    Get File    ${TEST_DATA}${/}hook-result.txt    encoding=UTF-8
    Should Contain    ${hook_result}    version=0.2.0
    Should Contain    ${hook_result}    command=add
    Should Contain    ${hook_result}    args=Hook argument with spaces

Relative Base Directory Invokes Hook Outside Base Directory
    Set Test Variable    ${TEST_DATA}    ${TEST_CWD}${/}data
    Create Directory    ${TEST_DATA}
    ${hook}=    Catenate    SEPARATOR=\n
    ...    @echo off
    ...    > hook-result.txt echo cwd=%CD%
    Create File    ${TEST_DATA}${/}record-hook.cmd    ${hook}${\n}    encoding=UTF-8
    Write Shiori Config    base_dir=data    hook=record-hook.cmd

    ${result}=    Run Shiori    add    Relative base directory hook
    Shiori Should Succeed    ${result}
    File Should Exist    ${TEST_DATA}${/}hook-result.txt
    ${hook_result}=    Get File    ${TEST_DATA}${/}hook-result.txt    encoding=UTF-8
    Should Contain    ${hook_result}    cwd=${TEST_DATA}

Failing Hook Does Not Undo Successful Command
    ${hook}=    Catenate    SEPARATOR=\n
    ...    @echo off
    ...    exit /b 7
    Create File    ${TEST_DATA}${/}failing-hook.cmd    ${hook}${\n}    encoding=UTF-8
    Write Shiori Config    hook=failing-hook.cmd

    ${result}=    Run Shiori    add    Persist despite hook failure
    Shiori Should Succeed    ${result}
    Combined Output Should Contain    ${result}    Exit code 7
    Data File Should Contain    NOTES.md    Persist despite hook failure
