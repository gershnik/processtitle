chcp 65001 && powershell -c "Get-WmiObject Win32_Process | Select-Object ProcessId, CommandLine"
