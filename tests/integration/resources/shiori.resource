*** Settings ***
Library    OperatingSystem
Library    Process
Library    String

*** Variables ***
${SHIORI_BINARY}    ${EXECDIR}${/}shiori.exe

*** Keywords ***
Create Isolated Test Workspace
    ${id}=      Evaluate    str(uuid.uuid4())    modules=uuid
    ${root}=    Join Path    ${OUTPUT DIR}    workspaces    ${id}
    ${root}=    Normalize Path    ${root}
    ${cwd}=     Join Path    ${root}    cwd
    ${home}=    Join Path    ${root}    home
    ${data}=    Join Path    ${root}    data
    Set Test Variable    ${TEST_ROOT}    ${root}
    Set Test Variable    ${TEST_CWD}     ${cwd}
    Set Test Variable    ${TEST_HOME}    ${home}
    Set Test Variable    ${TEST_DATA}    ${data}
    Create Directory    ${cwd}
    Create Directory    ${home}
    Create Directory    ${data}

Remove Isolated Test Workspace
    Remove Directory    ${TEST_ROOT}    recursive=True

Use Working Directory As Data Directory
    Set Test Variable    ${TEST_DATA}    ${TEST_CWD}

Write Shiori Config
    [Arguments]    ${base_dir}=${TEST_DATA}    ${hook}=${EMPTY}
    ${content}=    Catenate    SEPARATOR=\n
    ...    version: 1
    ...    base_dir: ${base_dir}
    IF    '${hook}' != '${EMPTY}'
        ${content}=    Catenate    SEPARATOR=\n    ${content}    hook_after_command: ${hook}
    END
    Create File    ${TEST_CWD}${/}.shiori    ${content}${\n}    encoding=UTF-8

Run Shiori
    [Arguments]    @{arguments}
    ${result}=    Run Process
    ...    ${SHIORI_BINARY}
    ...    @{arguments}
    ...    cwd=${TEST_CWD}
    ...    env:HOME=${TEST_HOME}
    ...    env:USERPROFILE=${TEST_HOME}
    ...    output_encoding=UTF-8
    ...    timeout=10s
    ...    on_timeout=terminate
    RETURN    ${result}

Shiori Should Succeed
    [Arguments]    ${result}
    Should Be Equal As Integers
    ...    ${result.rc}
    ...    0
    ...    msg=stdout:\n${result.stdout}\n\nstderr:\n${result.stderr}

Shiori Should Fail
    [Arguments]    ${result}
    Should Not Be Equal As Integers
    ...    ${result.rc}
    ...    0
    ...    msg=stdout:\n${result.stdout}\n\nstderr:\n${result.stderr}

Combined Output Should Contain
    [Arguments]    ${result}    ${expected}
    ${combined}=    Catenate    SEPARATOR=\n    ${result.stdout}    ${result.stderr}
    Should Contain    ${combined}    ${expected}

Data File Should Contain
    [Arguments]    ${filename}    ${expected}
    File Should Exist    ${TEST_DATA}${/}${filename}
    ${content}=    Get File    ${TEST_DATA}${/}${filename}    encoding=UTF-8
    Should Contain    ${content}    ${expected}

Data File Should Not Contain
    [Arguments]    ${filename}    ${unexpected}
    File Should Exist    ${TEST_DATA}${/}${filename}
    ${content}=    Get File    ${TEST_DATA}${/}${filename}    encoding=UTF-8
    Should Not Contain    ${content}    ${unexpected}

Data File Should Equal
    [Arguments]    ${filename}    ${expected}
    File Should Exist    ${TEST_DATA}${/}${filename}
    ${content}=    Get File    ${TEST_DATA}${/}${filename}    encoding=UTF-8
    Should Be Equal    ${content}    ${expected}

No Rewrite Artifacts Should Remain
    File Should Not Exist    ${TEST_DATA}${/}NOTES.md.tmp
    File Should Not Exist    ${TEST_DATA}${/}TODOS.md.tmp
    File Should Not Exist    ${TEST_DATA}${/}TODOS.md.bak
