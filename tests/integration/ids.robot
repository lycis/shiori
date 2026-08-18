*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Id Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Id Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory
    Write Shiori Config

*** Test Cases ***
Removed Todo Id Is Never Reused
    ${first}=    Run Shiori    todo    add    First task
    Shiori Should Succeed    ${first}
    ${remove}=    Run Shiori    todo    remove    0
    Shiori Should Succeed    ${remove}
    ${second}=    Run Shiori    todo    add    Second task
    Shiori Should Succeed    ${second}
    Data File Should Contain        TODOS.md    \#shiori/id/1
    Data File Should Not Contain    TODOS.md    \#shiori/id/0
    Data File Should Contain        TODOS.md    last_id: 2

Note Id Continues After Sequence Gap
    ${date}=       Evaluate    datetime.date.today().strftime("%Y-%m-%d")    modules=datetime
    ${id_date}=    Evaluate    datetime.date.today().strftime("%Y%m%d")    modules=datetime
    ${notes}=    Catenate    SEPARATOR=\n
    ...    ---
    ...    version: 1
    ...    ---
    ...
    ...    \# ${date}
    ...    * First note <!-- shiori:id=${id_date}-0001 -->
    ...    * Third note <!-- shiori:id=${id_date}-0003 -->
    Create File    ${TEST_DATA}${/}NOTES.md    ${notes}${\n}    encoding=UTF-8

    ${result}=    Run Shiori    add    Fourth note
    Shiori Should Succeed    ${result}
    Data File Should Contain    NOTES.md    Fourth note <!-- shiori:id=${id_date}-0004 -->

Unicode Base Directory With Spaces Supports Notes And Todos
    [Documentation]    Known defect on Windows: narrow C filesystem APIs cannot open UTF-8 paths.
    [Tags]    robot:skip    known-issue
    ${unicode_data}=    Join Path    ${TEST_ROOT}    Daten mit Leerzeichen 東京
    Create Directory    ${unicode_data}
    Set Test Variable    ${TEST_DATA}    ${unicode_data}
    Write Shiori Config

    ${note}=    Run Shiori    add    Unicode path note
    Shiori Should Succeed    ${note}
    ${todo}=    Run Shiori    todo    add    Unicode path todo
    Shiori Should Succeed    ${todo}
    Data File Should Contain    NOTES.md    Unicode path note
    Data File Should Contain    TODOS.md    Unicode path todo
    File Should Not Exist    ${TEST_CWD}${/}NOTES.md
    File Should Not Exist    ${TEST_CWD}${/}TODOS.md
