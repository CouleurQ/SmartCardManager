# SmartCardManager
Smart Card communication and Java Card management

## Development Platform
- Arch Linux x86_64 (04'2025)
- QT 6.9.0
- pcsc-lite 2.3.3

## Compilation
### Linux
```
mkdir -p build/release
cd $_
qmake ../../SmartCardManager.pro
make
```

### MacOS
```
mkdir -p build/release && cd build/release
qmake ../../SmartCardManager.pro CONFIG+=release
make
```
Uses the system PCSC framework.

### Windows
Qt 6 for MSVC, run from a Visual Studio developer prompt:
```
qmake SmartCardManager.pro CONFIG+=release
nmake release
```
Uses the system `winscard.dll`.

## Builds and releases
- Every push to `main` builds snapshot packages (Linux `.deb` and AppImage, macOS `.dmg`, Windows `.zip`),
  available as artifacts of the *Snapshot* workflow run.
- A release is published by pushing a version tag on `main`:
  ```
  git tag v0.2.0
  git push origin v0.2.0
  ```
  The *Release* workflow builds all packages from the tag and attaches them to a GitHub release. The tag
  (without the `v`) becomes the version shown in the About dialog; a tag with a suffix such as `v0.2.0-rc1`
  is published as a pre-release.


## Build script and tests

`./make.sh` builds with Qt 6 + pcsc-lite (`./make.sh deps` checks them), `./make.sh test` runs the
headless test suite. See `docs/ARCHITECTURE.md` and `docs/CODE_REVIEW.md`.
