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
`QApplication`. A known, not yet fixed defect is documented with `QEXPECT_FAIL`, so an `XPASS`
tells you it got fixed (remove the `QEXPECT_FAIL`). Tab handlers and `MainWindow` are reached through
`tests/privateaccess.h` (`#define private public`) because they expose no test seams.

| Test class | Covers |
|---|---|
| TstUtility | hex/byte conversion helpers |
| TstCommands | Command types, RunScriptModel (+ QAbstractItemModelTester), action strings |
| TstHexEdit | typing, formatting, delete/backspace, paste, maxBytes, focus (UI) |
| TstViews | the four command editor views (UI) |
| TstLogView | log rows, colours, clear (UI) |
| TstPcscTransport | PC/SC behaviour without reader/card |
| TstCardSession | session, async send, command execution (MockTransport) |
| TstScriptFile | script / command list files, command JSON, factory |
| TstSendCommand | Send-Command tab: add/edit/move/delete, send, load/save, drag&drop (UI) |
| TstRunScript | Run-Script tab: selection, editors, load/save/round trip (UI) |
| TstMainWindow | window structure, tab wiring, log dock |

## Module overview

```
src/
  main.cpp                 single-instance handling (QLocalServer), signal handling, wiring
  utility.*                namespace Utility: hex string <-> QByteArray helpers
  card/
    icardtransport.h       ICardTransport: readers/connect/disconnect/reset/transmit (+ log signal)
    pcsctransport.*        PcscTransport: pcsc-lite implementation
    atrparser.*            AtrParser::parse: ISO 7816-3 ATR decoding (interface/historical bytes, TCK)
    cardsession.*          CardSession: selected reader + connection state, serialises transport access,
                           sendAsync() runs transmissions on a worker thread
  commands/
    command.*              Command: abstract value type (typeName, clone, toJson/fromJson, execute)
    commandsendbytes.* commandapdu.* commandterminal.*   the three command types
    commandfactory.*       typeName -> new command
    runscriptmodel.h       RunScriptModel : QAbstractListModel of QSharedPointer<Command>
    commandview.*          editor widget base, one subclass per command type
    commandviewapdu.* commandviewsendbytes.* commandviewterminal.*   emit notifyUpdateCommand(cmd)
  io/
    scriptfile.*           Run Script file load/save
    commandlistfile.*      Send Command list load/save
    jsonwriter.h           ordered hand-written JSON helper
  controls/hexedit.*       QTextEdit: hex input with grouping, max length (promoted widget)
  ui/
    mainwindow.*           tabs, reader combo, connect button, log dock, status bar
    tabhandlersendcommand.*  "Send Command" tab controller (list of raw commands, send, load/save)
    tabhandlerrunscript.*    "Run Script" tab controller (command list + editor, load/save)
    atrview.*              "ATR" tab (shown while connected): colour coded ATR, summary, byte-by-byte tree
    logview.*              QTableView log of reader/card traffic, SVG direction icons
    aboutdialog.*
res/                       icons, forms, JSON schemas
.github/workflows/ci.yml   build + test on every push
```

### Runtime data flow

1. `main` creates a `PcscTransport`, a `CardSession` on top of it and the `MainWindow`, which receives the
   session by constructor (no global state). A second launch is forwarded to the first through a local socket.
2. `MainWindow` fills the reader combo from `session->readers()`; Connect/Disconnect/Reset call the session,
   which emits `stateChanged` so that toolbar and status bar follow.
3. Send tab: Send -> `session->sendAsync(bytes)`; the PC/SC call runs on the session's worker thread and the
   answer returns as `responseReceived` on the GUI thread -> `logBytes(CardRsp)` -> `MainWindow` -> `LogView`.
4. Run Script tab: list item selected -> `replaceCurrentView` builds the matching `CommandView*`; edits flow
   back through `notifyUpdateCommand` -> `model->setData(.., Qt::UserRole)`. `Command::execute(CardSession&)`
   runs through `ScriptRunner` on a worker thread (Run/Step/Stop); `CardSession` runs a monitor thread that detects card removal.
5. Tests use `MockTransport` (tests/mocktransport.h) instead of a reader.

### File formats

*Command list* (Send tab, schema `res/commandListScheme.json`): `version`, `dateTime`, `id`, `apduList`
of `{name, cmd(hex)}`.

*Script file* (Run Script tab, schema `res/scriptFileScheme.json`): `version`, `dateTime`, `id`, `items` of
`{name, seq, type, ...}` with `type` one of `SendBytes` (`cmd`, `expectedResponse`), `SendAPDU`
(`cla ins p1 p2 data le expectedResponse`) or `TerminalCmd` (`action`). Keys are written in this fixed order.
