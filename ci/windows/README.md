# Windows container build environment

This image installs Visual Studio 2022 Build Tools, MSVC v143, CMake, Git,
the Windows 10 19041 SDK, and the C++ UWP packaging tools. Build it and run it
from a Windows Server 2022 or compatible Windows 11 container host:

```powershell
docker build -m 8GB -t battleship-uwp-build -f ci/windows/Dockerfile .
docker run --rm -m 12GB `
  -v "${PWD}:C:\src" `
  -v "${PWD}\dist:C:\out" `
  battleship-uwp-build
```

The source mount must already contain recursively initialized submodules. The
image is intentionally a build environment, not a runtime image; its output is
a signed sideload package under `dist`.

Windows containers require a Windows container host and matching/compatible
host and image versions. A Linux Docker daemon cannot execute this image. The
GitHub Actions Windows runner is the default CI path when no Windows container
host is available.
