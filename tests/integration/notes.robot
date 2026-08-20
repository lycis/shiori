*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Note Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Note Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory
    Write Shiori Config

*** Test Cases ***
Adding A Note Creates Notes File And Stable Id
    ${result}=    Run Shiori    add    Integration test note
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    * Integration test note
    ${today}=    Evaluate    datetime.date.today().strftime("%Y%m%d")    modules=datetime
    Data File Should Contain    NOTES.md    <!-- shiori:id=${today}-0001 -->
    No Rewrite Artifacts Should Remain

Adding A Second Note Increments Daily Id
    ${first}=    Run Shiori    add    First note
    Shiori Should Succeed    ${first}
    ${second}=    Run Shiori    add    Second note
    Shiori Should Succeed    ${second}
    ${today}=    Evaluate    datetime.date.today().strftime("%Y%m%d")    modules=datetime
    Data File Should Contain    NOTES.md    <!-- shiori:id=${today}-0001 -->
    Data File Should Contain    NOTES.md    <!-- shiori:id=${today}-0002 -->

Topic Is Persisted With Note
    ${result}=    Run Shiori    add    --topic    testing    Topic note
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    Topic note #shiori/topic/testing

Unicode Note Survives Round Trip
    ${result}=    Run Shiori    add    Grüße from 🦊 東京
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    Grüße from 🦊 東京

Missing Note Text Does Not Modify Notes
    ${result}=    Run Shiori    add
    Shiori Should Fail    ${result}
    File Should Not Exist    ${TEST_DATA}${/}NOTES.md

Notes Are Written To Configured Base Directory
    Set Test Variable    ${TEST_DATA}    ${TEST_ROOT}${/}data
    Write Shiori Config
    ${result}=    Run Shiori    add    Stored outside cwd
    Shiori Should Succeed    ${result}
    File Should Exist        ${TEST_DATA}${/}NOTES.md
    File Should Not Exist    ${TEST_CWD}${/}NOTES.md

Repeated Adds To Configured Base Directory Preserve All Notes
    Set Test Variable    ${TEST_DATA}    ${TEST_ROOT}${/}data
    Write Shiori Config
    ${first}=    Run Shiori    add    First external note
    Shiori Should Succeed    ${first}
    ${second}=    Run Shiori    add    Second external note
    Shiori Should Succeed    ${second}
    Data File Should Contain    NOTES.md    First external note
    Data File Should Contain    NOTES.md    Second external note
    File Should Not Exist    ${TEST_CWD}${/}NOTES.md

Note Add Alias Creates A Note
    ${result}=    Run Shiori    note    add    Added through note command
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    * Added through note command
    ${today}=    Evaluate    datetime.date.today().strftime("%Y%m%d")    modules=datetime
    Data File Should Contain    NOTES.md    <!-- shiori:id=${today}-0001 -->

Note Retopic Changes An Existing Topic
    ${add}=    Run Shiori    note    add    --topic    original    Retopic this note
    Shiori Should Succeed    ${add}
    ${today}=    Evaluate    datetime.date.today().strftime("%Y%m%d")    modules=datetime
    ${result}=    Run Shiori    note    retopic    ${today}-0001    changed
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    * Retopic this note #shiori/topic/changed
    Data File Should Not Contain    NOTES.md    \#shiori/topic/original
    Data File Should Contain    NOTES.md    <!-- shiori:id=${today}-0001 -->
    No Rewrite Artifacts Should Remain

Note Retopic None Removes An Existing Topic
    ${add}=    Run Shiori    note    add    --topic    temporary    Remove this topic
    Shiori Should Succeed    ${add}
    ${today}=    Evaluate    datetime.date.today().strftime("%Y%m%d")    modules=datetime
    ${result}=    Run Shiori    note    retopic    ${today}-0001    none
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    * Remove this topic
    Data File Should Not Contain    NOTES.md    \#shiori/topic/
    Data File Should Contain    NOTES.md    <!-- shiori:id=${today}-0001 -->
    No Rewrite Artifacts Should Remain

Note Retopic Missing Id Leaves Notes Unchanged
    ${add}=    Run Shiori    note    add    --topic    original    Preserve this note
    Shiori Should Succeed    ${add}
    ${before}=    Get File    ${TEST_DATA}${/}NOTES.md    encoding=UTF-8
    ${result}=    Run Shiori    note    retopic    missing-id    changed
    Shiori Should Fail    ${result}
    Data File Should Equal    NOTES.md    ${before}
    No Rewrite Artifacts Should Remain

Note Rewrite Refuses To Drop Unexpected Markdown
    ${notes}=    Catenate    SEPARATOR=\n
    ...    ---
    ...    version: 1
    ...    ---
    ...
    ...    # 2030-04-05
    ...    * Managed note <!-- shiori:id=20300405-0001 -->
    ...    This manually written paragraph must survive.
    Create File    ${TEST_DATA}${/}NOTES.md    ${notes}${\n}    encoding=UTF-8
    ${before}=    Get File    ${TEST_DATA}${/}NOTES.md    encoding=UTF-8

    ${result}=    Run Shiori    note    retopic    20300405-0001    changed
    Shiori Should Fail    ${result}
    Data File Should Equal    NOTES.md    ${before}

Successful Note Rewrite Keeps Recovery Backup
    ${add}=    Run Shiori    note    add    --topic    original    Preserve complete file
    Shiori Should Succeed    ${add}
    ${before}=    Get File    ${TEST_DATA}${/}NOTES.md    encoding=UTF-8
    ${today}=    Evaluate    datetime.date.today().strftime("%Y%m%d")    modules=datetime

    ${result}=    Run Shiori    note    retopic    ${today}-0001    changed
    Shiori Should Succeed    ${result}
    ${backup}=    Get File    ${TEST_DATA}${/}NOTES.md.bak    encoding=UTF-8
    Should Be Equal    ${backup}    ${before}

Note Help Lists New Commands
    ${result}=    Run Shiori    note    help
    Shiori Should Succeed    ${result}
    Combined Output Should Contain    ${result}    add
    Combined Output Should Contain    ${result}    retopic
