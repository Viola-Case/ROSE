$choices = @(
    [System.Management.Automation.Host.ChoiceDescription]::new('&Yes', 'Generate source docs')
    [System.Management.Automation.Host.ChoiceDescription]::new('&No', 'Only generate API docs')
)

$result = $host.UI.PromptForChoice('', 'Generate source docs?', $choices, 1)


$err_result = & doxygen ./api.Doxyfile 2>&1

if ($LASTEXITCODE -ne 0) { 
    Write-Error "API docs generation failed: $err_result" 
} else {
    Write-Output "API docs generated"
}

if ($result -eq 0) {
    $err_result = & doxygen ./src.Doxyfile 2>&1
    if ($LASTEXITCODE -ne 0) { 
        Write-Error "Source docs generation failed: $err_result" 
    } else {
        Write-Output "Source docs generated"
    }
}