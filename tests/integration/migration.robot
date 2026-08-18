*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Migration Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Migration Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory
    Write Shiori Config

*** Test Cases ***
Notes Migration Preserves Data And Is Idempotent
    ${legacy}=    Catenate    SEPARATOR=\n
    ...    \# 2030-04-05
    ...    * Existing note #shiori/topic/project <!-- shiori:id=20300405-0003 -->
    ...    * Note without id #shiori/topic/project
    ...
    ...    \# 2030-04-06
    ...    * Other date note
    Create File    ${TEST_DATA}${/}NOTES.md    ${legacy}${\n}    encoding=UTF-8

    ${first}=    Run Shiori    util    migrate
    Shiori Should Succeed    ${first}
    Data File Should Contain    NOTES.md    version: 1
    Data File Should Contain    NOTES.md    Existing note #shiori/topic/project <!-- shiori:id=20300405-0003 -->
    Data File Should Contain    NOTES.md    Note without id #shiori/topic/project <!-- shiori:id=20300405-0004 -->
    Data File Should Contain    NOTES.md    Other date note <!-- shiori:id=20300406-0001 -->
    ${after_first}=    Get File    ${TEST_DATA}${/}NOTES.md    encoding=UTF-8

    ${second}=    Run Shiori    util    migrate
    Shiori Should Succeed    ${second}
    Data File Should Equal    NOTES.md    ${after_first}
    No Rewrite Artifacts Should Remain

Malformed Notes Migration Leaves Source Unchanged
    ${malformed}=    Catenate    SEPARATOR=\n
    ...    ---
    ...    version: 0
    ...    \# 2030-04-05
    ...    * Existing note
    Create File    ${TEST_DATA}${/}NOTES.md    ${malformed}${\n}    encoding=UTF-8
    ${before}=    Get File    ${TEST_DATA}${/}NOTES.md    encoding=UTF-8

    ${result}=    Run Shiori    util    migrate
    Shiori Should Fail    ${result}
    Data File Should Equal    NOTES.md    ${before}
    No Rewrite Artifacts Should Remain
