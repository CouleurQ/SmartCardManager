#include "hexedit.h"

#include <QKeyEvent>
#include <QMimeData>
#include <QTextCursor>


HexEdit::HexEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setAcceptRichText(false);
    maxBytes = 0xFFFF;
}


void HexEdit::setMaxBytes(const int max)
{
    maxBytes = max;
}


int HexEdit::length() const
{
    return QByteArray::fromHex(toPlainText().replace(" ", "").toUtf8()).length();
}


QByteArray HexEdit::data() const
{
    return QByteArray::fromHex(toPlainText().replace(" ", "").toUtf8());
}


void HexEdit::setData(const QByteArray &data)
{
    setPlainText(QString::fromLatin1(data.toHex(' ').toUpper()));
}


void HexEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Tab ||
        event->key() == Qt::Key_Backtab) {
        reformatText();
        event->ignore();
        return;
    }

    if (event->matches(QKeySequence::Copy) ||
        event->matches(QKeySequence::Paste) ||
        event->matches(QKeySequence::Cut) ||
        event->matches(QKeySequence::SelectAll)) {
        QTextEdit::keyPressEvent(event);
        reformatText();
        return;
    }

    const QString input = event->text().toUpper();
    const bool isShortcut = event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);

    if (event->key() == Qt::Key_Backspace) {
        handleBackspace();
    } else if (event->key() == Qt::Key_Delete) {
        handleDelete();
    } else if (input.isEmpty() || isShortcut) {
        QTextEdit::keyPressEvent(event);
    } else if (isHexDigit(input[0])) {
        insertNibbles(input.left(1));
    }
    // every other printable key (including Return) is ignored
}


void HexEdit::insertFromMimeData(const QMimeData *source)
{
    insertNibbles(source->text().toUpper());
}


void HexEdit::focusInEvent(QFocusEvent *event)
{
    QTextEdit::focusInEvent(event);
    selectAll();
}


void HexEdit::focusOutEvent(QFocusEvent *event)
{
    QTextEdit::focusOutEvent(event);
    QTextCursor cursor = textCursor();
    cursor.clearSelection();  // Ensure the selection is cleared
    setTextCursor(cursor);
}


bool HexEdit::isHexDigit(QChar chr)
{
    return chr.isDigit() || (chr >= 'A' && chr <= 'F');
}


int HexEdit::nibbleIndex(int textPosition)
{
    return textPosition - textPosition / 3;
}


int HexEdit::textPosition(int nibbleIndex)
{
    return nibbleIndex + nibbleIndex / 2;
}


QString HexEdit::rawNibbles() const
{
    return toPlainText().remove(' ');
}


void HexEdit::applyRaw(const QString &raw, int cursorNibble)
{
    QString formatted;
    for (int i = 0; i < raw.length(); i++) {
        if (i > 0 && i % 2 == 0)
            formatted += ' ';
        formatted += raw[i];
    }

    if (formatted != toPlainText()) {
        QTextCursor all(document());
        all.beginEditBlock();
        all.select(QTextCursor::Document);
        all.insertText(formatted);
        all.endEditBlock();
    }

    QTextCursor cursor(document());
    cursor.setPosition(qBound(0, textPosition(cursorNibble), formatted.length()));
    setTextCursor(cursor);
}


void HexEdit::insertNibbles(const QString &text)
{
    QString digits;
    for (QChar chr : text) {
        if (isHexDigit(chr))
            digits += chr;
    }

    QTextCursor cursor = textCursor();
    int from = nibbleIndex(cursor.selectionStart());
    int to = nibbleIndex(cursor.selectionEnd());

    QString raw = rawNibbles();
    raw.remove(from, to - from);

    const int capacity = qMax(0, maxBytes * 2 - raw.length());
    digits.truncate(capacity);

    raw.insert(from, digits);
    applyRaw(raw, from + digits.length());
}


void HexEdit::handleBackspace()
{
    QTextCursor cursor = textCursor();
    int from = nibbleIndex(cursor.selectionStart());
    int to = nibbleIndex(cursor.selectionEnd());

    if (from == to && from > 0)
        from--;

    QString raw = rawNibbles();
    raw.remove(from, to - from);
    applyRaw(raw, from);
}


void HexEdit::handleDelete()
{
    QTextCursor cursor = textCursor();
    int from = nibbleIndex(cursor.selectionStart());
    int to = nibbleIndex(cursor.selectionEnd());

    QString raw = rawNibbles();
    if (from == to && to < raw.length())
        to++;

    raw.remove(from, to - from);
    applyRaw(raw, from);
}


void HexEdit::reformatText()
{
    applyRaw(rawNibbles(), nibbleIndex(textCursor().position()));
}
