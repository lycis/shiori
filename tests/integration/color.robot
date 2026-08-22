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
