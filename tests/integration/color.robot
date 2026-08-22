*** Settings ***
Resource         resources/shiori.resource
Test Setup       Create Isolated Test Workspace
Test Teardown    Remove Isolated Test Workspace

*** Test Cases ***
Interactive Output Is Colored By Default
    Write Shiori Config
    ${result}=    Run Shiori    today
    Shiori Should Succeed    ${result}
    Output Should Contain ANSI Sequence    ${result.stdout}

Error Diagnostics Are Colored By Default
    ${result}=    Run Shiori    unknown-command
    Shiori Should Fail    ${result}
    Output Should Contain ANSI Sequence    ${result.stderr}

Configuration Can Disable Color
    Write Shiori Config    color=false
    ${result}=    Run Shiori    today
    Shiori Should Succeed    ${result}
    Output Should Not Contain ANSI Sequence    ${result.stdout}

No Color Switch Overrides Configuration
    Write Shiori Config    color=true
    ${result}=    Run Shiori    --no-color    today
    Shiori Should Succeed    ${result}
    Output Should Not Contain ANSI Sequence    ${result.stdout}

No Color Switch Disables Error Diagnostics
    ${result}=    Run Shiori    --no-color    unknown-command
    Shiori Should Fail    ${result}
    Output Should Not Contain ANSI Sequence    ${result.stderr}

NO_COLOR Overrides Configuration
    Write Shiori Config    color=true
    ${result}=    Run Shiori With NO_COLOR    1    today
    Shiori Should Succeed    ${result}
    Output Should Not Contain ANSI Sequence    ${result.stdout}

Empty NO_COLOR Preserves Configured Color
    Write Shiori Config    color=true
    ${result}=    Run Shiori With NO_COLOR    ${EMPTY}    today
    Shiori Should Succeed    ${result}
    Output Should Contain ANSI Sequence    ${result.stdout}
