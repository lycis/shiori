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
    ...    ---${\n}version: 1${\n}---${\n}${\n}# 2030-04-05${\n}* Architecture decision #decision #work #work #shiori/topic/project <!-- shiori:id=20300405-0001 -->${\n}* Follow up #work #urgent <!-- shiori:id=20300405-0002 -->${\n}* Alpha plan #shiori/topic/Project%20Alpha <!-- shiori:id=20300405-0003 -->${\n}* Café plan #shiori/topic/Caf%C3%A9%20Planning <!-- shiori:id=20300405-0004 -->${\n}
    Create File
    ...    ${TEST_DATA}${/}TODOS.md
    ...    ---${\n}version: 1${\n}last_id: 3${\n}---${\n}${\n}* [ ] Due item #work #shiori/id/0 #shiori/created/2030-04-01 #shiori/due/2030-04-05 #shiori/topic/project${\n}* [/] Active item #urgent #shiori/id/1 #shiori/created/2030-04-02 #shiori/topic/Todo%20Only${\n}* [X] Finished item #work #shiori/id/2 #shiori/created/2030-04-03${\n}

*** Test Cases ***
Today Supports Explicit Date
    ${result}=    Run Shiori    today    --date    2030-04-05
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    2030-04-05
    Should Contain    ${result.stdout}    Architecture decision
    Should Contain    ${result.stdout}    Due item
    Should Contain    ${result.stdout}    🪧 project
    Should Contain    ${result.stdout}    Active item
    Should Not Contain    ${result.stdout}    Finished item

Topic Lists Matching Notes And Todos
    ${result}=    Run Shiori    topic    project
    Shiori Should Succeed    ${result}
    Should Contain        ${result.stdout}    Architecture decision
    Should Contain        ${result.stdout}    Due item
    Should Not Contain    ${result.stdout}    Follow up
    Should Not Contain    ${result.stdout}    Active item

Topic Finds A Todo Only Topic
    ${result}=    Run Shiori    topic    Todo Only
    Shiori Should Succeed    ${result}
    Should Contain        ${result.stdout}    Active item
    Should Not Contain    ${result.stdout}    Due item

Topic Finds A Multi Word Encoded Topic
    ${result}=    Run Shiori    topic    Project Alpha
    Shiori Should Succeed    ${result}
    Should Contain        ${result.stdout}    Alpha plan
    Should Not Contain    ${result.stdout}    Architecture decision

Topic Finds A UTF-8 Multi Word Encoded Topic
    ${result}=    Run Shiori    topic    Café Planning
    Shiori Should Succeed    ${result}
    Should Contain        ${result.stdout}    Café plan
    Should Not Contain    ${result.stdout}    Alpha plan

Topic List Displays Canonical Decoded Names
    ${result}=    Run Shiori    topic    --list
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    project
    Should Contain    ${result.stdout}    Project Alpha
    Should Contain    ${result.stdout}    Café Planning
    Should Contain    ${result.stdout}    Todo Only
    Should Match Regexp    ${result.stdout}    (?s).*project.*2 items.*\\(1 note, 1 todo\\).*
    Should Match Regexp    ${result.stdout}    (?s).*Todo Only.*1 item.*\\(0 notes, 1 todo\\).*
    Should Not Contain    ${result.stdout}    Project%20Alpha
    Should Not Contain    ${result.stdout}    Caf%C3%A9%20Planning

Tag Requires Every Requested Tag
    ${result}=    Run Shiori    tag    work    urgent
    Shiori Should Succeed    ${result}
    Should Contain        ${result.stdout}    Follow up
    Should Not Contain    ${result.stdout}    Architecture decision
    Should Not Contain    ${result.stdout}    Due item

Tag List Counts Notes And Todos Once Per Item
    ${result}=    Run Shiori    tag    --list
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    \#decision
    Should Match Regexp    ${result.stdout}    (?s).*\#decision.*1 item.*\\(1 note, 0 todos\\).*
    Should Contain    ${result.stdout}    \#urgent
    Should Match Regexp    ${result.stdout}    (?s).*\#urgent.*2 items.*\\(1 note, 1 todo\\).*
    Should Contain    ${result.stdout}    \#work
    Should Match Regexp    ${result.stdout}    (?s).*\#work.*4 items.*\\(2 notes, 2 todos\\).*
    Should Not Contain    ${result.stdout}    \#shiori/
    Should Match Regexp    ${result.stdout}    (?s).*\#decision.*\#urgent.*\#work.*

Tag List Short Option Is Discoverable
    ${list}=    Run Shiori    tag    -l
    Shiori Should Succeed    ${list}
    Should Contain    ${list.stdout}    🏷️ Tags

    ${help}=    Run Shiori    tag    --help
    Shiori Should Succeed    ${help}
    Should Contain    ${help.stdout}    -l, --list
