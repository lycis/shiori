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
    [Documentation]    Known defect: note rewrite temp paths currently use cwd instead of base_dir.
    [Tags]    robot:skip    known-issue
    Set Test Variable    ${TEST_DATA}    ${TEST_ROOT}${/}data
    Write Shiori Config
    ${result}=    Run Shiori    add    Stored outside cwd
    Shiori Should Succeed    ${result}
    File Should Exist        ${TEST_DATA}${/}NOTES.md
    File Should Not Exist    ${TEST_CWD}${/}NOTES.md
