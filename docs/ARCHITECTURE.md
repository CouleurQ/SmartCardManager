# SmartCardManager – Internal Documentation

Qt 6 Widgets desktop tool to talk to PC/SC smart card readers: send raw APDUs, keep lists of
commands, and (work in progress) run scripts of commands.

## Build and test

```
./make.sh deps          # check qmake6, g++, pkg-config, libpcsclite
./make.sh               # release build  -> build/release/SmartCardManager
./make.sh -d            # debug build
./make.sh run           # build + start
./make.sh test          # build + run the QtTest suite (offscreen, no reader needed)
./make.sh test -- TstHexEdit -v2    # one test class, verbose QtTest output
./make.sh clean
```

Tests live in `tests/` (`tests.pro`, one executable `tst_smartcardmanager`). Each test file registers
its class via `TestRegistrar<T>` (`tests/testregistry.h`); `tests/main.cpp` runs them all inside a
`ScmApplication`. Known defects are documented with `QEXPECT_FAIL` and a `B-xx` id from
`CODE_REVIEW.md`, so the suite is green today and an `XPASS` tells you a bug got fixed
(remove the `QEXPECT_FAIL`). Tab handlers and `MainWindow` are reached through
`tests/privateaccess.h` (`#define private public`) because they expose no test seams.

| Test class | Covers |
|---|---|
| TstUtility | hex/byte conversion helpers |
| TstCommands | Command types, RunScriptModel (+ QAbstractItemModelTester), action strings |
| TstHexEdit | typing, formatting, delete/backspace, paste, maxBytes, focus (UI) |
| TstViews | the four command editor views (UI) |
| TstLogView | log rows, colours, clear (UI) |
| TstPcscWrapper | behaviour without reader/card |
| TstSendCommand | Send-Command tab: add/edit/move/delete, send, load/save, drag&drop (UI) |
| TstRunScript | Run-Script tab: selection, editors, load/save/round trip (UI) |
| TstMainWindow | window structure, tab wiring, log dock |

## Module overview

```
src/
  main.cpp                 single-instance handling (QLocalServer), app start
  scmapplication.*         QApplication subclass; owns global `pcsc` (PcscWrapper*) and `cardReader`
  pcscwrapper.*            thin C++ wrapper over pcsc-lite (context, readers, connect, transmit, reset)
  utility.*                hex string <-> QByteArray helpers
  commands/
    command.h              Command base (QObject) + CommandSendBytes / CommandApdu / CommandTerminal
    model.h                RunScriptModel : QAbstractListModel of QSharedPointer<Command>
    view.* viewapdu.*      editor widgets, one per command type; emit notifyUpdateCommand(cmd)
    viewsendbytes.* viewterminal.*
  controls/hexedit.h       header-only QTextEdit: hex input with grouping, max length (promoted widget)
  ui/
    mainwindow.*           tabs, reader combo, connect button, log dock, status bar
    tabhandlersendcommand.*  "Send Command" tab controller (list of raw commands, send, JSON load/save)
    tabhandlerrunscript.*    "Run Script" tab controller (command list + editor, JSON load/save)
    logview.*              QTableView log of reader/card traffic, SVG direction icons
    aboutdialog.*
res/                       icons, default JSON files, `commandListScheme.json`
```

### Runtime data flow

1. `main` creates `ScmApplication` (which creates `PcscWrapper`) and `MainWindow`; a second launch
   forwards its arguments to the first through a local socket.
2. `MainWindow` fills the reader combo from `pcsc->getReaders()`; the Connect button calls
   `pcsc->connect(reader)` / `disconnect()`.
3. Tab handlers get the wrapper by downcasting `qApp` to `ScmApplication`.
4. Send tab: Send -> `pcsc->send(reader, bytes)` -> signals `logBytes(CardCmd/CardRsp, ...)` ->
   `MainWindow` forwards to `LogView`.
5. Run Script tab: list item selected -> `replaceCurrentView` builds the matching `View*`; edits flow
   back through `notifyUpdateCommand` -> `model->setData(.., Qt::UserRole)`. Script execution is **not
   implemented yet** (`expectedResponse` is stored but unused).

### File formats

*Command list* (Send tab, schema `res/commandListScheme.json`): version + array of `{name, data(hex)}`.

*Script file* (Run Script tab, no schema yet): array of commands with `type`
(`SendBytes`/`SendAPDU`/`Terminal`), `name`, `seq`, type-specific fields. Note B-01 (APDU payload key
mismatch) and B-02 (hand-written JSON, no escaping).

## Conventions worth knowing

* Command objects are shared (`QSharedPointer<Command>`) between model and editor; editors work on
  a copy and emit the replaced command.
* `HexEdit` normalises content to upper-case, space separated bytes; `data()` returns the bytes.
* `LogType` drives both the SVG icon and the colour of a log row.
