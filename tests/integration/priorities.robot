*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Priority Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Priority Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory
    Write Shiori Config

*** Test Cases ***
Priorities Round Trip And Preserve Other Fields
    FOR    ${priority}    IN    high    medium    low    none
        ${result}=    Run Shiori    todo    add    --priority    ${priority}    --topic    work    --due    tomorrow    Task ${priority} \#work
        Shiori Should Succeed    ${result}
    END
    FOR    ${id}    ${priority}    IN    0    high    1    medium    2    low    3    none
        ${result}=    Run Shiori    todo    show    ${id}
        Shiori Should Succeed    ${result}
        Should Contain    ${result.stdout}    Priority: ${priority}
    END
    ${result}=    Run Shiori    todo    rewrite    0    -p    low
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    rewrite    0    Replacement \#work    --due    none
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    start    0
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    show    0
    Should Contain    ${result.stdout}    Priority: low
    Should Contain    ${result.stdout}    Topic: work
    Should Contain    ${result.stdout}    Due: none
    ${result}=    Run Shiori    todo    rewrite    0    --priority    none
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    show    0
    Should Contain    ${result.stdout}    Priority: none
    No Rewrite Artifacts Should Remain

Priority Filters Combine And Keep File Order
    ${result}=    Run Shiori    todo    add    -p    low    First \#work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    -p    high    Second \#work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    -p    high    Third \#home
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    Legacy
    Shiori Should Succeed    ${result}
    ${before}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8
    ${result}=    Run Shiori    todo    list
    Shiori Should Succeed    ${result}
    Should Match Regexp    ${result.stdout}    (?s)First.*Second.*Third.*Legacy
    ${result}=    Run Shiori    todo    list    --priority    high    --tag    work    --no-due-date    --open
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    Second
    Should Contain    ${result.stdout}    [high]
    Should Not Contain    ${result.stdout}    First
    Should Not Contain    ${result.stdout}    Third
    ${result}=    Run Shiori    todo    list    --priority    none
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    Legacy
    Should Not Contain    ${result.stdout}    Second
    ${after}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8
    Should Be Equal    ${before}    ${after}

Invalid Priority Arguments Do Not Change Storage
    ${result}=    Run Shiori    todo    add    Existing
    Shiori Should Succeed    ${result}
    ${before}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8
    FOR    ${command}    IN    add    list
        ${result}=    Run Shiori    todo    ${command}    --priority    urgent
        Shiori Should Fail    ${result}
        ${result}=    Run Shiori    todo    ${command}    --priority
        Shiori Should Fail    ${result}
        ${result}=    Run Shiori    todo    ${command}    -p    high    --priority    low
        Shiori Should Fail    ${result}
    END
    ${result}=    Run Shiori    todo    rewrite    0    --priority    urgent
    Shiori Should Fail    ${result}
    ${result}=    Run Shiori    todo    rewrite    0    --priority
    Shiori Should Fail    ${result}
    ${after}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8
    Should Be Equal    ${before}    ${after}

Malformed Priority Metadata Prevents Rewrites
    FOR    ${metadata}    IN    urgent    high \#shiori/priority/low    high-extra
        ${content}=    Catenate    SEPARATOR=\n    ---    version: 1    last_id: 1    ---    * [ ] Existing \#shiori/id/0 \#shiori/created/2026-01-01 \#shiori/priority/${metadata}
        Create File    ${TEST_DATA}${/}TODOS.md    ${content}${\n}    encoding=UTF-8
        ${before}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8
        ${result}=    Run Shiori    todo    rewrite    0    Replacement
        Shiori Should Fail    ${result}
        ${after}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8
        Should Be Equal    ${before}    ${after}
    END

Priority Appears In Other Todo Views
    ${result}=    Run Shiori    add    Seed note \#work
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    add    --priority    medium    --topic    project    Visible \#work
    Shiori Should Succeed    ${result}
    FOR    ${command}    ${argument}    IN    topic    project    tag    work
        ${result}=    Run Shiori    ${command}    ${argument}
        Shiori Should Succeed    ${result}
        Should Contain    ${result.stdout}    [medium]
    END
    ${result}=    Run Shiori    today
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    [medium]
    ${result}=    Run Shiori    capture    --help
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    --priority

Priority Survives Lifecycle And Pruning Other Todos
    ${result}=    Run Shiori    todo    add    -p    high    Retained
    Shiori Should Succeed    ${result}
    FOR    ${command}    IN    start    done    reopen    cancel    reopen    defer    reopen
        ${result}=    Run Shiori    todo    ${command}    0
        Shiori Should Succeed    ${result}
        ${result}=    Run Shiori    todo    show    0
        Shiori Should Succeed    ${result}
        Should Contain    ${result.stdout}    Priority: high
    END
    ${result}=    Run Shiori    todo    add    Completed
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    done    1
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    prune    --force
    Shiori Should Succeed    ${result}
    Data File Should Contain    TODOS.md    \#shiori/priority/high
    ${result}=    Run Shiori    todo    rewrite    0    Changed    --due    tomorrow    --priority    low
    Shiori Should Succeed    ${result}
    ${result}=    Run Shiori    todo    show    0
    Should Contain    ${result.stdout}    Priority: low
    Should Contain    ${result.stdout}    Text: Changed

Legacy Text Is Not Interpreted As Priority Metadata
    ${content}=    Catenate    SEPARATOR=\n    ---    version: 1    last_id: 1    ---    * [ ] Explain \#shiori/priority/high \#shiori/id/0 \#shiori/created/2026-01-01
    Create File    ${TEST_DATA}${/}TODOS.md    ${content}${\n}    encoding=UTF-8
    ${result}=    Run Shiori    todo    show    0
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    Priority: none
    Should Contain    ${result.stdout}    Text: Explain \#shiori/priority/high
    ${result}=    Run Shiori    todo    start    0
    Shiori Should Succeed    ${result}
    Data File Should Contain    TODOS.md    Explain \#shiori/priority/high \#shiori/id/0
