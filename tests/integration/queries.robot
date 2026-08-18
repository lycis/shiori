*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Query Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Query Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory
    Write Shiori Config
    Create File
    ...    ${TEST_DATA}${/}NOTES.md
    ...    ---${\n}version: 1${\n}---${\n}${\n}# 2030-04-05${\n}* Architecture decision #decision #work #shiori/topic/project <!-- shiori:id=20300405-0001 -->${\n}* Follow up #work #urgent <!-- shiori:id=20300405-0002 -->${\n}
    Create File
    ...    ${TEST_DATA}${/}TODOS.md
    ...    ---${\n}version: 1${\n}last_id: 3${\n}---${\n}${\n}* [ ] Due item #work #shiori/id/0 #shiori/created/2030-04-01 #shiori/due/2030-04-05${\n}* [/] Active item #urgent #shiori/id/1 #shiori/created/2030-04-02${\n}* [X] Finished item #work #shiori/id/2 #shiori/created/2030-04-03${\n}

*** Test Cases ***
Today Supports Explicit Date
    ${result}=    Run Shiori    today    --date    2030-04-05
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    2030-04-05
    Should Contain    ${result.stdout}    Architecture decision
    Should Contain    ${result.stdout}    Due item
    Should Contain    ${result.stdout}    Active item
    Should Not Contain    ${result.stdout}    Finished item

Topic Lists Matching Notes
    ${result}=    Run Shiori    topic    project
    Shiori Should Succeed    ${result}
    Should Contain        ${result.stdout}    Architecture decision
    Should Not Contain    ${result.stdout}    Follow up

Tag Requires Every Requested Tag
    ${result}=    Run Shiori    tag    work    urgent
    Shiori Should Succeed    ${result}
    Should Contain        ${result.stdout}    Follow up
    Should Not Contain    ${result.stdout}    Architecture decision
    Should Not Contain    ${result.stdout}    Due item

