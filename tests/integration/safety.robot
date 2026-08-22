*** Settings ***
Resource         resources/shiori.resource
Test Setup       Prepare Safety Workspace
Test Teardown    Remove Isolated Test Workspace

*** Keywords ***
Prepare Safety Workspace
    Create Isolated Test Workspace
    Use Working Directory As Data Directory
    Write Shiori Config

*** Test Cases ***
Invalid Todo Date Leaves Existing File Unchanged
    ${seed}=    Run Shiori    todo    add    Existing task
    Shiori Should Succeed    ${seed}
    ${before}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8

    ${result}=    Run Shiori    todo    add    --due    not-a-date    Invalid task
    Shiori Should Fail    ${result}
    Data File Should Equal    TODOS.md    ${before}
    No Rewrite Artifacts Should Remain

Missing Todo Id Leaves Existing File Unchanged
    ${seed}=    Run Shiori    todo    add    Existing task
    Shiori Should Succeed    ${seed}
    ${before}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8

    ${result}=    Run Shiori    todo    rewrite    999    Changed text
    Shiori Should Fail    ${result}
    Data File Should Equal    TODOS.md    ${before}
    No Rewrite Artifacts Should Remain

Malformed Todo Front Matter Fails Without Rewriting
    ${malformed}=    Catenate    SEPARATOR=\n
    ...    ---
    ...    version: 1
    ...    last_id: 1
    ...    * [ ] Existing task #shiori/id/0 #shiori/created/2030-01-01
    Create File    ${TEST_DATA}${/}TODOS.md    ${malformed}${\n}    encoding=UTF-8
    ${before}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8

    ${result}=    Run Shiori    todo    rewrite    0    Changed text
    Shiori Should Fail    ${result}
    Data File Should Equal    TODOS.md    ${before}
    No Rewrite Artifacts Should Remain

Malformed Todo Item Fails Without Rewriting
    ${malformed}=    Catenate    SEPARATOR=\n
    ...    ---
    ...    version: 1
    ...    last_id: 1
    ...    ---
    ...
    ...    * [?] Existing task #shiori/id/0 #shiori/created/2030-01-01
    Create File    ${TEST_DATA}${/}TODOS.md    ${malformed}${\n}    encoding=UTF-8
    ${before}=    Get File    ${TEST_DATA}${/}TODOS.md    encoding=UTF-8

    ${result}=    Run Shiori    todo    done    0
    Shiori Should Fail    ${result}
    Data File Should Equal    TODOS.md    ${before}
    No Rewrite Artifacts Should Remain

Malformed Notes Front Matter Fails Without Rewriting
    ${malformed}=    Catenate    SEPARATOR=\n
    ...    ---
    ...    version: 1
    ...    # 2030-01-01
    ...    * Existing note
    Create File    ${TEST_DATA}${/}NOTES.md    ${malformed}${\n}    encoding=UTF-8
    ${before}=    Get File    ${TEST_DATA}${/}NOTES.md    encoding=UTF-8

    ${result}=    Run Shiori    add    New note
    Shiori Should Fail    ${result}
    Data File Should Equal    NOTES.md    ${before}
    No Rewrite Artifacts Should Remain

Invalid Calendar Date Is Rejected
    ${result}=    Run Shiori    todo    add    --due    2030-02-31    Impossible date
    Shiori Should Fail    ${result}
    File Should Not Exist    ${TEST_DATA}${/}TODOS.md

Invalid Month Lengths And Leap Days Are Rejected
    FOR    ${date}    IN    2030-04-31    2100-02-29
        ${result}=    Run Shiori    todo    add    --due    ${date}    Impossible date
        Shiori Should Fail    ${result}
        File Should Not Exist    ${TEST_DATA}${/}TODOS.md
    END

Valid Month Boundaries And Leap Days Are Accepted
    FOR    ${date}    IN    2030-01-31    2030-04-30    2028-02-29    2000-02-29
        ${result}=    Run Shiori    todo    add    --due    ${date}    Valid date
        Shiori Should Succeed    ${result}
        Data File Should Contain    TODOS.md    \#shiori/due/${date}
    END

Absolute Dates Require Strict Syntax
    FOR    ${date}    IN    2030-2-03    2030-02-03junk
        ${result}=    Run Shiori    todo    add    --due    ${date}    Invalid date
        Shiori Should Fail    ${result}
        File Should Not Exist    ${TEST_DATA}${/}TODOS.md
    END

Relative Dates Remain Supported
    FOR    ${date}    IN    today    yesterday    tomorrow
        ${result}=    Run Shiori    todo    add    --due    ${date}    Relative date
        Shiori Should Succeed    ${result}
    END
