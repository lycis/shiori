*** Settings ***
Resource         resources/shiori.resource
Test Setup       Create Isolated Test Workspace
Test Teardown    Remove Isolated Test Workspace

*** Test Cases ***
Init Creates Local Configuration
    ${result}=    Run Shiori    init
    Shiori Should Succeed    ${result}
    File Should Exist    ${TEST_CWD}${/}.shiori
    ${config}=    Get File    ${TEST_CWD}${/}.shiori    encoding=UTF-8
    Should Contain    ${config}    version: 1
    Should Contain    ${config}    base_dir: ${TEST_CWD}
    Should Contain    ${config}    color: true
    Should Contain    ${config}    notes_filename: NOTES.md
    Should Contain    ${config}    todo_filename: TODOS.md

Init Refuses To Overwrite Configuration
    ${first}=     Run Shiori    init
    Shiori Should Succeed    ${first}
    ${before}=    Get File    ${TEST_CWD}${/}.shiori    encoding=UTF-8

    ${second}=    Run Shiori    init
    Shiori Should Fail    ${second}
    Combined Output Should Contain    ${second}    already exists
    ${after}=    Get File    ${TEST_CWD}${/}.shiori    encoding=UTF-8
    Should Be Equal    ${after}    ${before}

Reinit Replaces Existing Configuration
    Create File    ${TEST_CWD}${/}.shiori    invalid configuration
    ${result}=    Run Shiori    init    --reinit
    Shiori Should Succeed    ${result}
    ${config}=    Get File    ${TEST_CWD}${/}.shiori    encoding=UTF-8
    Should Contain        ${config}    version: 1
    Should Not Contain    ${config}    invalid configuration

Local Configuration Takes Precedence Over Home Configuration
    Create File
    ...    ${TEST_HOME}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_HOME}${\n}
    Write Shiori Config
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    base_dir: ${TEST_DATA}

Home Configuration Is Used As Fallback
    Create File
    ...    ${TEST_HOME}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_DATA}${\n}
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    base_dir: ${TEST_DATA}

Missing Color Setting Defaults To True
    Write Shiori Config
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    color: true

Color Setting Accepts False
    Write Shiori Config    color=false
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    color: false

Invalid Color Setting Is Rejected
    Write Shiori Config    color=sometimes
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    color must be true or false

Relative Storage Filenames Are Accepted
    Write Shiori Config    notes=journal\\notes.markdown    todos=tasks.txt
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    notes_filename: journal\\notes.markdown
    Should Contain    ${result.stdout}    todo_filename: tasks.txt

Equivalent Storage Paths Are Rejected
    Write Shiori Config    notes=data.md    todos=DATA.md
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    storage paths conflict

Storage Paths Cannot Escape Base Directory
    Write Shiori Config    notes=..\\outside.md
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    invalid path component

Absolute Storage Paths Are Rejected
    Write Shiori Config    todos=C:\\absolute\\todos.md
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    must be relative to base_dir

Storage Paths Cannot Conflict With Rewrite Artifacts
    Write Shiori Config    notes=tasks.md.bak    todos=tasks.md
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    storage paths conflict

Explicitly Empty Storage Filename Is Rejected
    Create File
    ...    ${TEST_CWD}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_DATA}${\n}notes_filename: ""${\n}
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    notes_filename must not be empty

Nested Configuration Accepts Flexible Indentation
    Create File
    ...    ${TEST_CWD}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_DATA}${\n}hooks:${\n} after_command: ""${\n}
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}

Legacy Flat Hook Configuration Remains Supported
    Create File
    ...    ${TEST_CWD}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_DATA}${\n}hook_after_command: ""${\n}
    ${result}=    Run Shiori    config    show
    Shiori Should Succeed    ${result}

Legacy And Nested Hook Keys Conflict
    Create File
    ...    ${TEST_CWD}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_DATA}${\n}hook_after_command: old.cmd${\n}hooks:${\n}  after_command: new.cmd${\n}
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    cannot both be configured

Unknown Configuration Key Is Rejected
    Create File
    ...    ${TEST_CWD}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_DATA}${\n}colro: false${\n}
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    unknown key 'colro'

Malformed Configuration Reports Its Location
    Create File
    ...    ${TEST_CWD}${/}.shiori
    ...    version: 1${\n}base_dir: ${TEST_DATA}${\n}hooks:${\n}  missing colon${\n}
    ${result}=    Run Shiori    config    show
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    .shiori:4:3
