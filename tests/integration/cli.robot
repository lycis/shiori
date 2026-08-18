*** Settings ***
Resource         resources/shiori.resource
Test Setup       Create Isolated Test Workspace
Test Teardown    Remove Isolated Test Workspace

*** Test Cases ***
Help Does Not Require Configuration
    ${result}=    Run Shiori    help
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    Available commands:
    Should Contain    ${result.stdout}    todo
    Should Contain    ${result.stdout}    tag

Version Does Not Require Configuration
    ${result}=    Run Shiori    version
    Shiori Should Succeed    ${result}
    Should Match Regexp    ${result.stdout}    (?m)^shiori [0-9]+\.[0-9]+\.[0-9]+
    Should Contain    ${result.stdout}    platform:

No Command Returns Dedicated Exit Code
    ${result}=    Run Shiori
    Should Be Equal As Integers    ${result.rc}    2
    Combined Output Should Contain    ${result}    No command provided

Unknown Command Fails
    ${result}=    Run Shiori    does-not-exist
    Should Be Equal As Integers    ${result.rc}    3
    Combined Output Should Contain    ${result}    Unknown command

Command Requiring Missing Configuration Fails
    ${result}=    Run Shiori    add    unreachable note
    Shiori Should Fail    ${result}
    Combined Output Should Contain    ${result}    config file not found

Completion Script Can Be Generated Without Configuration
    Write Shiori Config
    ${result}=    Run Shiori    util    completion    powershell
    Shiori Should Succeed    ${result}
    Should Contain    ${result.stdout}    Register-ArgumentCompleter
    Should Contain    ${result.stdout}    'todo'
