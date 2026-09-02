*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Alias Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Alias Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory

Write Alias Configuration
    [Arguments]    ${alias_entries}    ${hook}=${EMPTY}
    ${content}=    Catenate    SEPARATOR=\n
    ...    version: 1
    ...    base_dir: ${TEST_DATA}
    ...    aliases:
    ...    ${alias_entries}
    IF    $hook
        ${content}=    Catenate    SEPARATOR=\n
        ...    ${content}
        ...    hooks:
        ...    ${SPACE}${SPACE}after_command: ${hook}
    END
    Create File    ${TEST_CWD}${/}.shiori    ${content}${\n}    encoding=UTF-8

*** Test Cases ***
Single Word Alias Resolves Built In
    Write Alias Configuration    ${SPACE}${SPACE}h: help
    ${result}=    Run Shiori    h
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    usage:

Alias Prepends Fixed Arguments And Preserves Trailing Arguments
    Write Alias Configuration    ${SPACE}${SPACE}quick: 'add "fixed words"'
    ${result}=    Run Shiori    quick    trailing words
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    fixed words trailing words

Alias Chains Resolve Iteratively
    ${entries}=    Catenate    SEPARATOR=\n
    ...    ${SPACE}${SPACE}first: second
    ...    ${SPACE}${SPACE}second: add chained
    Write Alias Configuration    ${entries}
    ${result}=    Run Shiori    first
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    chained

Built In Commands Take Precedence Over Aliases
    Write Alias Configuration    ${SPACE}${SPACE}help: missing-command
    ${result}=    Run Shiori    help
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    usage:

Direct Alias Recursion Is Rejected
    Write Alias Configuration    ${SPACE}${SPACE}loop: loop
    ${result}=    Run Shiori    loop
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    Recursive command alias detected: loop -> loop

Indirect Alias Recursion Shows Path
    ${entries}=    Catenate    SEPARATOR=\n
    ...    ${SPACE}${SPACE}one: two
    ...    ${SPACE}${SPACE}two: three
    ...    ${SPACE}${SPACE}three: one
    Write Alias Configuration    ${entries}
    ${result}=    Run Shiori    one
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    one -> two -> three -> one

Unknown Alias Target Uses Unknown Command Error
    Write Alias Configuration    ${SPACE}${SPACE}bad: nowhere
    ${result}=    Run Shiori    bad
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    Unknown command: nowhere

Home Configuration Provides Aliases As Fallback
    Remove File    ${TEST_CWD}${/}.shiori
    ${content}=    Catenate    SEPARATOR=\n
    ...    version: 1
    ...    base_dir: ${TEST_DATA}
    ...    aliases:
    ...    ${SPACE}${SPACE}h: help
    Create File    ${TEST_HOME}${/}.shiori    ${content}${\n}    encoding=UTF-8
    ${result}=    Run Shiori    h
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    usage:

Alias Values Must Be Strings
    Write Alias Configuration    ${SPACE}${SPACE}bad: true
    ${result}=    Run Shiori    bad
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    aliases.bad must be a string

Alias Expansions Must Not Be Whitespace
    Write Alias Configuration    ${SPACE}${SPACE}bad: '${SPACE}${SPACE}${SPACE}'
    ${result}=    Run Shiori    bad
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    aliases.bad must not be empty

Nested Alias Entries Are Rejected
    ${entries}=    Catenate    SEPARATOR=\n
    ...    ${SPACE}${SPACE}group:
    ...    ${SPACE}${SPACE}${SPACE}${SPACE}bad: help
    Write Alias Configuration    ${entries}
    ${result}=    Run Shiori    group
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    aliases must be direct entries

Alias Count Is Bounded
    ${entries}=    Set Variable    ${EMPTY}
    FOR    ${index}    IN RANGE    33
        ${entry}=    Set Variable    ${SPACE}${SPACE}alias${index}: help
        ${entries}=    Catenate    SEPARATOR=\n    ${entries}    ${entry}
    END
    Write Alias Configuration    ${entries}
    ${result}=    Run Shiori    alias0
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    no more than 32 aliases

Expanded Argument Count Is Bounded
    ${expansion}=    Evaluate    'help ' + ' '.join(f'arg{i}' for i in range(65))
    Write Alias Configuration    ${SPACE}${SPACE}huge: ${expansion}
    ${result}=    Run Shiori    huge
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    maximum of 64 arguments

Config Show Includes Aliases
    Write Alias Configuration    ${SPACE}${SPACE}tasks: todo list
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    aliases:
    Should Contain    ${result.stdout}    tasks: todo list

Hook Receives Expanded Command And Arguments Once
    ${platform}=    Evaluate    platform.system()    modules=platform
    Skip If    '${platform}' != 'Windows'    Hook execution is currently implemented only on Windows.
    ${hook}=    Catenate    SEPARATOR=\n
    ...    @echo off
    ...    >> hook-result.txt echo command=%SHIORI_COMMAND%
    ...    >> hook-result.txt echo args=%SHIORI_COMMAND_ARGS%
    Create File    ${TEST_DATA}${/}record-hook.cmd    ${hook}${\n}    encoding=UTF-8
    Write Alias Configuration    ${SPACE}${SPACE}remember: add fixed    record-hook.cmd
    ${result}=    Run Shiori    remember    trailing
    Shiori Should Succeed    ${result}
    ${hook_result}=    Get File    ${TEST_DATA}${/}hook-result.txt    encoding=UTF-8
    Should Contain    ${hook_result}    command=add
    Should Contain    ${hook_result}    args=fixed trailing
