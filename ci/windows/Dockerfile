# escape=`

FROM mcr.microsoft.com/windows/servercore:ltsc2022

SHELL ["cmd", "/S", "/C"]

ADD https://aka.ms/vs/17/release/vs_buildtools.exe C:\TEMP\vs_buildtools.exe

RUN start /w C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache `
        --installPath C:\BuildTools `
        --add Microsoft.VisualStudio.Workload.VCTools `
        --add Microsoft.VisualStudio.ComponentGroup.UWP.VC.BuildTools `
        --add Microsoft.VisualStudio.Component.Windows10SDK.19041 `
        --add Microsoft.VisualStudio.Component.VC.CMake.Project `
        --add Microsoft.VisualStudio.Component.TeamExplorer.MinGit `
        --includeRecommended `
    || IF "%ERRORLEVEL%"=="3010" EXIT 0

RUN del /q C:\TEMP\vs_buildtools.exe

WORKDIR C:\src
ENTRYPOINT ["C:\\BuildTools\\Common7\\Tools\\VsDevCmd.bat", "&&"]
CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass", "-File", "ci\\windows\\build-uwp.ps1"]
