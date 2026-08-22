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

Todo List Filters Due Date With Calendar Boundaries
    ${today}=          Evaluate    datetime.date.today()    modules=datetime
    ${weekday}=        Evaluate    $today.weekday()
    ${monday}=         Evaluate    $today - datetime.timedelta(days=$weekday)    modules=datetime
    ${sunday}=         Evaluate    $monday + datetime.timedelta(days=6)    modules=datetime
    ${before_week}=    Evaluate    $monday - datetime.timedelta(days=1)    modules=datetime
    ${after_week}=     Evaluate    $sunday + datetime.timedelta(days=1)    modules=datetime
    ${today_text}=     Evaluate    str($today)
    ${monday_text}=    Evaluate    str($monday)
    ${sunday_text}=    Evaluate    str($sunday)
    ${before_text}=    Evaluate    str($before_week)
    ${after_text}=     Evaluate    str($after_week)

    ${result}=    Run Shiori    todo    add    --due    ${before_text}    Before week #work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    --due    ${monday_text}    Monday boundary #work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    --due    ${today_text}    Due today #work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    --due    ${sunday_text}    Sunday boundary #work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    --due    ${after_text}    After week #work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    Unscheduled #work
    Shiori Should Succeed    ${result}

    ${today_result}=    Run Shiori    todo    list    --due-today
    Shiori Should Succeed    ${today_result}
    Should Contain        ${today_result.stdout}    Due today #work
    Should Not Contain    ${today_result.stdout}    Unscheduled #work

    ${overdue_result}=    Run Shiori    todo    list    --overdue
    Shiori Should Succeed    ${overdue_result}
    Should Contain        ${overdue_result.stdout}    Before week #work
    Should Not Contain    ${overdue_result.stdout}    Due today #work

    ${week_result}=    Run Shiori    todo    list    --due-this-week
    Shiori Should Succeed    ${week_result}
    Should Contain        ${week_result.stdout}    Monday boundary #work
    Should Contain        ${week_result.stdout}    Sunday boundary #work
    Should Not Contain    ${week_result.stdout}    Before week #work
    Should Not Contain    ${week_result.stdout}    After week #work

    ${none_result}=    Run Shiori    todo    list    --no-due-date
    Shiori Should Succeed    ${none_result}
    Should Contain        ${none_result.stdout}    Unscheduled #work
    Should Not Contain    ${none_result.stdout}    Due today #work

    ${conflicting}=    Run Shiori    todo    list    --overdue    --due-today
    Shiori Should Fail    ${conflicting}
    Combined Output Should Contain    ${conflicting}    Date filters are mutually exclusive

Todo Date Filters Compose With Status And Tags
    ${yesterday}=         Evaluate    str(datetime.date.today() - datetime.timedelta(days=1))    modules=datetime
    ${result}=    Run Shiori    todo    add    --due    ${yesterday}    Open overdue #work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    --due    ${yesterday}    Done overdue #work #urgent
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    done    1
    Shiori Should Succeed    ${result}

    ${default}=    Run Shiori    todo    list    --overdue
    Shiori Should Succeed    ${default}
    Should Contain        ${default.stdout}    Open overdue #work
    Should Not Contain    ${default.stdout}    Done overdue #work #urgent

    ${combined}=    Run Shiori    todo    list    --done    --overdue    --tag    work    --tag    urgent
    Shiori Should Succeed    ${combined}
    Should Contain        ${combined.stdout}    Done overdue #work #urgent
    Should Not Contain    ${combined.stdout}    Open overdue #work

Prune Requires Force And Preserves Id Counter
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
    Set Test Variable    ${TEST_DATA}    ${TEST_ROOT}${/}data
    Write Shiori Config
    ${add}=    Run Shiori    todo    add    Stored outside cwd
    Shiori Should Succeed    ${add}
    File Should Exist        ${TEST_DATA}${/}TODOS.md
    File Should Not Exist    ${TEST_CWD}${/}TODOS.md

    ${start}=    Run Shiori    todo    start    0
    Shiori Should Succeed    ${start}
    Data File Should Contain    TODOS.md    * [/] Stored outside cwd

    ${rewrite}=    Run Shiori    todo    rewrite    0    Rewritten outside cwd
    Shiori Should Succeed    ${rewrite}
    Data File Should Contain    TODOS.md    Rewritten outside cwd

    ${done}=    Run Shiori    todo    done    0
    Shiori Should Succeed    ${done}
    Data File Should Contain    TODOS.md    * [x] Rewritten outside cwd

    ${remove}=    Run Shiori    todo    remove    0
    Shiori Should Succeed    ${remove}
    Data File Should Not Contain    TODOS.md    Rewritten outside cwd
    File Should Not Exist    ${TEST_CWD}${/}TODOS.md
    No Rewrite Artifacts Should Remain

New Todo File Uses Current Format Version
    ${result}=    Run Shiori    todo    add    Versioned task
    Shiori Should Succeed    ${result}
    Data File Should Contain    TODOS.md    version: 1

Status Change For Missing Todo Fails
    ${seed}=    Run Shiori    todo    add    Existing task
    Shiori Should Succeed    ${seed}
    ${result}=    Run Shiori    todo    start    999
    Shiori Should Fail    ${result}
