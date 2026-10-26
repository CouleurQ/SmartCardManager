#include "testregistry.h"
#include "hexedit.h"

#include <QTest>
#include <QSignalSpy>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>

// Exposes the protected paste entry point used by the context menu and drag & drop.
struct HexEditProbe : HexEdit {
    using HexEdit::insertFromMimeData;
};

class TstHexEdit : public QObject
{
    Q_OBJECT

private:
    HexEditProbe *edit = nullptr;

private slots:
    void init();
    void cleanup();

    void setDataFormatsWithSpaces();
    void dataAndLength();
    void typingFormatsAndUppercases();
    void nonHexKeysAreIgnored();
    void maxBytesLimitsTyping();
    void maxBytesLimitsTypingInTheMiddle();
    void typingReplacesSelection();
    void typingInTheMiddleKeepsFormatting();
    void backspaceRemovesLastNibble();
    void backspaceAcrossByteBoundary();
    void deleteInTheMiddle();
    void deleteAtStartRemovesFirstNibble();
    void textChangedIsEmittedWhenTyping();
    void tabKeyIsNotConsumed();
    void focusInSelectsAll();
    void pasteViaMimeDataFiltersAndFormats();
    void pasteViaMimeDataHonoursMaxBytes();
    void pasteViaKeyboardShortcutReformats();
    void dataWithOddNibble();
};

void TstHexEdit::init()
{
    edit = new HexEditProbe;
    edit->show();
    QVERIFY(QTest::qWaitForWindowExposed(edit));
    edit->setFocus();
}

void TstHexEdit::cleanup()
{
    delete edit;
    edit = nullptr;
}

void TstHexEdit::setDataFormatsWithSpaces()
{
    edit->setData(QByteArray::fromHex("00a4040c"));
    QCOMPARE(edit->toPlainText(), QString("00 A4 04 0C"));

    edit->setData(QByteArray());
    QCOMPARE(edit->toPlainText(), QString());
}

void TstHexEdit::dataAndLength()
{
    edit->setData(QByteArray::fromHex("00A404"));
    QCOMPARE(edit->data(), QByteArray::fromHex("00A404"));
    QCOMPARE(edit->length(), 3);
}

void TstHexEdit::typingFormatsAndUppercases()
{
    QTest::keyClicks(edit, "00a4040c");
    QCOMPARE(edit->toPlainText(), QString("00 A4 04 0C"));
    QCOMPARE(edit->data(), QByteArray::fromHex("00A4040C"));
}

void TstHexEdit::nonHexKeysAreIgnored()
{
    QTest::keyClicks(edit, "xyz!g 0");
    QCOMPARE(edit->toPlainText(), QString("0"));
}

void TstHexEdit::maxBytesLimitsTyping()
{
    edit->setMaxBytes(2);
    QTest::keyClicks(edit, "AABBCCDD");
    QCOMPARE(edit->toPlainText(), QString("AA BB"));
    QCOMPARE(edit->length(), 2);
}

// BUG: the limit check appends the new nibble at the END of the text regardless of the cursor,
// so inserting in the middle of a full field overflows maxBytes.
void TstHexEdit::maxBytesLimitsTypingInTheMiddle()
{
    edit->setMaxBytes(2);
    QTest::keyClicks(edit, "AABB");
    QTest::keyClick(edit, Qt::Key_Home);
    QTest::keyClick(edit, 'C');

    QVERIFY(edit->length() <= 2);
}

void TstHexEdit::typingReplacesSelection()
{
    edit->setData(QByteArray::fromHex("00A4"));
    edit->selectAll();
    QTest::keyClick(edit, 'B');
    QCOMPARE(edit->toPlainText(), QString("B"));
}

void TstHexEdit::typingInTheMiddleKeepsFormatting()
{
    QTest::keyClicks(edit, "0000");          // "00 00"
    QTest::keyClick(edit, Qt::Key_Home);
    QTest::keyClick(edit, 'A');              // "A0 00 0"
    QCOMPARE(edit->toPlainText(), QString("A0 00 0"));
    QCOMPARE(edit->textCursor().position(), 1);
}

void TstHexEdit::backspaceRemovesLastNibble()
{
    QTest::keyClicks(edit, "00A4");
    QTest::keyClick(edit, Qt::Key_Backspace);
    QCOMPARE(edit->toPlainText(), QString("00 A"));
    QTest::keyClick(edit, Qt::Key_Backspace);
    QCOMPARE(edit->toPlainText(), QString("00"));
}

void TstHexEdit::backspaceAcrossByteBoundary()
{
    QTest::keyClicks(edit, "00A4");          // "00 A4", cursor at 5
    QTest::keyClick(edit, Qt::Key_Left);
    QTest::keyClick(edit, Qt::Key_Left);
    QTest::keyClick(edit, Qt::Key_Left);     // cursor at 2 (between "00" and " ")
    QTest::keyClick(edit, Qt::Key_Backspace);
    QCOMPARE(edit->toPlainText(), QString("0A 4"));
}

void TstHexEdit::deleteInTheMiddle()
{
    QTest::keyClicks(edit, "00A4");          // "00 A4"
    QTest::keyClick(edit, Qt::Key_Home);
    QTest::keyClick(edit, Qt::Key_Right);    // cursor at 1
    QTest::keyClick(edit, Qt::Key_Delete);
    QCOMPARE(edit->toPlainText(), QString("0A 4"));
}

// BUG: handleDelete() only acts for position > 0, so Delete never removes the first nibble.
void TstHexEdit::deleteAtStartRemovesFirstNibble()
{
    QTest::keyClicks(edit, "00A4");
    QTest::keyClick(edit, Qt::Key_Home);
    QTest::keyClick(edit, Qt::Key_Delete);

    QCOMPARE(edit->toPlainText(), QString("0A 4"));
}

void TstHexEdit::textChangedIsEmittedWhenTyping()
{
    QSignalSpy changed(edit, &QTextEdit::textChanged);
    QTest::keyClicks(edit, "00");
    QVERIFY(changed.count() >= 1);
}

void TstHexEdit::tabKeyIsNotConsumed()
{
    edit->setData(QByteArray::fromHex("00"));
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    press.setAccepted(true);
    QApplication::sendEvent(edit, &press);
    QVERIFY2(!press.isAccepted(), "Tab must bubble up so focus can move to the next field");
    QCOMPARE(edit->toPlainText(), QString("00"));
}

void TstHexEdit::focusInSelectsAll()
{
    edit->setData(QByteArray::fromHex("00A4"));
    edit->clearFocus();
    edit->textCursor().clearSelection();
    QVERIFY(!edit->textCursor().hasSelection());

    edit->setFocus();
    QCOMPARE(edit->textCursor().selectedText(), QString("00 A4"));
}

void TstHexEdit::pasteViaMimeDataFiltersAndFormats()
{
    QMimeData mime;
    mime.setText("00 a4 zz 04");
    edit->insertFromMimeData(&mime);
    QCOMPARE(edit->toPlainText(), QString("00 A4 04"));
}

// BUG: insertFromMimeData() (context-menu paste, drag&drop) does not honour maxBytes.
void TstHexEdit::pasteViaMimeDataHonoursMaxBytes()
{
    edit->setMaxBytes(1);
    QMimeData mime;
    mime.setText("00A404");
    edit->insertFromMimeData(&mime);

    QVERIFY(edit->length() <= 1);
}

void TstHexEdit::pasteViaKeyboardShortcutReformats()
{
    QApplication::clipboard()->setText("00a404");
    edit->selectAll();
    QTest::keyClick(edit, Qt::Key_V, Qt::ControlModifier);
    QCOMPARE(edit->toPlainText(), QString("00 A4 04"));
}

// A lone nibble: HexEdit::data() and Utility::hexStringToByteArray() disagree (see tst_utility).
void TstHexEdit::dataWithOddNibble()
{
    QTest::keyClicks(edit, "A");
    QCOMPARE(edit->toPlainText(), QString("A"));
    qInfo() << "HexEdit::data() for a lone nibble 'A':" << edit->data().toHex()
            << "- Utility::hexStringToByteArray gives" << QByteArray::fromHex("A0").toHex();
    QCOMPARE(edit->data().length(), 1);
}

static TestRegistrar<TstHexEdit> registrar;
#include "tst_hexedit.moc"
