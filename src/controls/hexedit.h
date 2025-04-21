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
        QString input = event->text().toUpper();
        if (input.isEmpty()) {
            QTextEdit::keyPressEvent(event);  // Allow control keys (Backspace, arrows, etc.)
            return;
        }

        QChar ch = input[0];
        if (ch.isDigit() || (ch >= 'A' && ch <= 'F')) {
            insertHexChar(ch);
        } else if (event->key() == Qt::Key_Backspace) {
            handleBackspace();
        } else if (event->key() == Qt::Key_Delete) {
            // TODO
        }
    }

    void insertFromMimeData(const QMimeData* source) override
    {
        QString pasted = source->text().toUpper();
        QString clean;
        for (QChar ch : pasted) {
            if (ch.isDigit() || (ch >= 'A' && ch <= 'F')) {
                clean += ch;
            }
        }
        for (QChar ch : clean) {
            insertHexChar(ch);
        }
    }

private:
    void insertHexChar(QChar ch)
    {
        QString current = toPlainText();
        QString raw = current;
        raw.remove(' ');

        raw.append(ch);

        // Reformat: insert space every 2 characters
        QString formatted;
        for (int i = 0; i < raw.length(); i++) {
            if (i > 0 && i % 2 == 0) formatted += ' ';
            formatted += raw[i];
        }

        // Block signals and update
        this->blockSignals(true);
        this->setPlainText(formatted);
        this->blockSignals(false);

        // Move cursor to end
        QTextCursor cursor = this->textCursor();
        cursor.movePosition(QTextCursor::End);
        this->setTextCursor(cursor);
    }

    void handleBackspace() {
        QString current = toPlainText();
        QString raw = current;
        raw.remove(' ');
        if (!raw.isEmpty()) {
            raw.chop(1);
        }

        // Reformat again
        QString formatted;
        for (int i = 0; i < raw.length(); i++) {
            if (i > 0 && i % 2 == 0) formatted += ' ';
            formatted += raw[i];
        }

        this->blockSignals(true);
        this->setPlainText(formatted);
        this->blockSignals(false);

        QTextCursor cursor = this->textCursor();
        cursor.movePosition(QTextCursor::End);
        this->setTextCursor(cursor);
    }
};

#endif
