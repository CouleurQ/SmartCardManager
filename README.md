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
> TODO

### Windows
> TODO


## Build script and tests

`./make.sh` builds with Qt 6 + pcsc-lite (`./make.sh deps` checks them), `./make.sh test` runs the
headless test suite. See `docs/ARCHITECTURE.md` and `docs/CODE_REVIEW.md`.
