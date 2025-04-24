#ifndef HEXEDIT_H
#define HEXEDIT_H

#include <QTextEdit>
#include <QKeyEvent>
#include <QMimeData>
#include <QRegularExpression>


class HexEdit : public QTextEdit {

    Q_OBJECT

public:
    explicit HexEdit(QWidget* parent = nullptr) : QTextEdit(parent) {
        setAcceptRichText(false);
    }

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->matches(QKeySequence::Copy) ||
            event->matches(QKeySequence::Paste) ||
            event->matches(QKeySequence::Cut) ||
            event->matches(QKeySequence::SelectAll)) {
            QTextEdit::keyPressEvent(event);
            reformatText();
            return;
        }

        QString input = event->text().toUpper();
        if (input.isEmpty()) {
            QTextEdit::keyPressEvent(event);
            return;
        }

        QChar chr = input[0];
        if (chr.isDigit() || (chr >= 'A' && chr <= 'F')) {
            insertHexChar(chr);
        } else if (event->key() == Qt::Key_Backspace) {
            handleBackspace();
        } else if (event->key() == Qt::Key_Delete) {
            handleDelete();
        }
    }

    void insertFromMimeData(const QMimeData* source) override
    {
        for (QChar chr : source->text().toUpper()) {
            if (chr.isDigit() || (chr >= 'A' && chr <= 'F'))
                insertHexChar(chr);
        }
    }

private:
    void insertHexChar(QChar ch)
    {
        QTextCursor cursor = textCursor();
        int position = cursor.position();

        // Insert the hex character
        cursor.insertText(ch);

        reformatText();

        if ((position % 3) == 2) {
            cursor.setPosition(position + 2);  // Position after byte (space added)
        } else {
            cursor.setPosition(position + 1);  // Position after nibble
        }
        setTextCursor(cursor);
    }

    void handleBackspace() {
        QTextCursor cursor = textCursor();
        int position = cursor.position();
        int newPosition = position;

        // Check if there is any selected text and delete it
        if (cursor.hasSelection()) {
            cursor.removeSelectedText();  // Remove selected text
        } else {
            if (position > 0) {
                cursor.deletePreviousChar();
                newPosition--;

                // Check if the previous character is a space
                if (position % 3 != 2) {
                    cursor.deletePreviousChar();
                    newPosition--;
                }
            }
        }

        reformatText();

        cursor.setPosition((newPosition > 0) ? newPosition : 0);
        setTextCursor(cursor);
    }

    void handleDelete() {
        QTextCursor cursor = textCursor();
        int position = cursor.position();
        int newPosition = position;

        // Check if there is any selected text and delete it
        if (cursor.hasSelection()) {
            cursor.removeSelectedText();  // Remove selected text
        } else {
            if (position > 0) {
                qDebug() << position << " - " << position % 3;
                cursor.deleteChar();

                // Check if the previous character is a space
                if (position % 3 == 2)
                    cursor.deleteChar();
            }
        }

        reformatText();

        cursor.setPosition(newPosition);
        setTextCursor(cursor);
    }

    void reformatText() {
        // properly re-format with the spaces
        QString raw = toPlainText().remove(' ');
        QString formatted;
        for (int i = 0; i < raw.length(); i++) {
            if (i > 0 && i % 2 == 0) formatted += ' ';
            formatted += raw[i];
        }
        blockSignals(true);
        setPlainText(formatted);
        blockSignals(false);
    }
};

#endif
