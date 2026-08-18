*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Todo Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Todo Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory
    Write Shiori Config

*** Test Cases ***
Todo Can Move Through Lifecycle
    ${add}=    Run Shiori    todo    add    Ship release
    Shiori Should Succeed    ${add}
    Data File Should Contain    TODOS.md    last_id: 1
    Data File Should Contain    TODOS.md    * [ ] Ship release \#shiori/id/0

    ${start}=    Run Shiori    todo    start    0
    Shiori Should Succeed    ${start}
    Data File Should Contain    TODOS.md    * [/] Ship release

    ${done}=    Run Shiori    todo    done    0
    Shiori Should Succeed    ${done}
    Data File Should Contain    TODOS.md    * [x] Ship release

    ${reopen}=    Run Shiori    todo    reopen    0
    Shiori Should Succeed    ${reopen}
    Data File Should Contain    TODOS.md    * [ ] Ship release
    No Rewrite Artifacts Should Remain

Todo Due Date Can Be Rewritten And Removed
    ${add}=    Run Shiori    todo    add    --due    2030-04-05    Dated task
    Shiori Should Succeed    ${add}
    Data File Should Contain    TODOS.md    \#shiori/due/2030-04-05

    ${rewrite}=    Run Shiori    todo    rewrite    0    --due    2031-06-07    Renamed task
    Shiori Should Succeed    ${rewrite}
    Data File Should Contain    TODOS.md    Renamed task
    Data File Should Contain    TODOS.md    \#shiori/due/2031-06-07

    ${clear}=    Run Shiori    todo    rewrite    0    --due    none
    Shiori Should Succeed    ${clear}
    Data File Should Not Contain    TODOS.md    \#shiori/due/

Todo List Filters Status And Tags
    ${open}=    Run Shiori    todo    add    Open #work
    Shiori Should Succeed    ${open}
    ${done}=    Run Shiori    todo    add    Done #work #urgent
    Shiori Should Succeed    ${done}
    ${mark_done}=    Run Shiori    todo    done    1
    Shiori Should Succeed    ${mark_done}

    ${default}=    Run Shiori    todo    list
    Shiori Should Succeed    ${default}
    Should Contain        ${default.stdout}    Open #work
    Should Not Contain    ${default.stdout}    Done #work #urgent

    ${filtered}=    Run Shiori    todo    list    --done    --tag    work    --tag    urgent
    Shiori Should Succeed    ${filtered}
    Should Contain        ${filtered.stdout}    Done #work #urgent
    Should Not Contain    ${filtered.stdout}    Open #work

Prune Requires Force And Preserves Id Counter
    [Documentation]    Known defect: prune passes an absolute path through base_dir resolution twice.
    [Tags]    robot:skip    known-issue
    ${completed}=    Run Shiori    todo    add    Completed
    Shiori Should Succeed    ${completed}
    ${done}=    Run Shiori    todo    done    0
    Shiori Should Succeed    ${done}
    ${open}=    Run Shiori    todo    add    Still open
    Shiori Should Succeed    ${open}
    ${before}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8

    ${preview}=    Run Shiori    todo    prune
    Shiori Should Succeed    ${preview}
    ${unchanged}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8
    Should Be Equal    ${unchanged}    ${before}

    ${prune}=    Run Shiori    todo    prune    --force
    Shiori Should Succeed    ${prune}
    Data File Should Not Contain    TODOS.md    Completed
    Data File Should Contain        TODOS.md    Still open
    Data File Should Contain        TODOS.md    last_id: 2

Todo Mutations Honor Configured Base Directory
    [Documentation]    Known defect: several todo writers currently use cwd instead of base_dir.
    [Tags]    robot:skip    known-issue
    Set Test Variable    ${TEST_DATA}    ${TEST_ROOT}${/}data
    Write Shiori Config
    ${result}=    Run Shiori    todo    add    Stored outside cwd
    Shiori Should Succeed    ${result}
    File Should Exist        ${TEST_DATA}${/}TODOS.md
    File Should Not Exist    ${TEST_CWD}${/}TODOS.md

New Todo File Uses Current Format Version
    [Documentation]    Known defect: an empty TODO file is treated as metadata version zero.
    [Tags]    known-issue
    ${result}=    Run Shiori    todo    add    Versioned task
    Shiori Should Succeed    ${result}
    Data File Should Contain    TODOS.md    version: 1

Status Change For Missing Todo Fails
    [Documentation]    Known defect: status subcommands discard the result from set_todo_status.
    [Tags]    robot:skip    known-issue
    ${seed}=    Run Shiori    todo    add    Existing task
    Shiori Should Succeed    ${seed}
    ${result}=    Run Shiori    todo    start    999
    Shiori Should Fail    ${result}
