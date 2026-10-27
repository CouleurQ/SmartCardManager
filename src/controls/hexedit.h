#ifndef HEXEDIT_H
#define HEXEDIT_H

#include <QTextEdit>


// Text edit for hex bytes: only hex digits are accepted and shown grouped as "AA BB CC".
class HexEdit : public QTextEdit {

    Q_OBJECT

public:
    explicit HexEdit(QWidget *parent = nullptr);

    void setMaxBytes(const int max);
    int length() const;
    QByteArray data() const;
    void setData(const QByteArray &data);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    static bool isHexDigit(QChar chr);
    // The text is "AA BB CC": every third character is a space, so a text position maps to a nibble index.
    static int nibbleIndex(int textPosition);
    static int textPosition(int nibbleIndex);

    QString rawNibbles() const;
    // Replaces the content with the grouped form of raw (keeps the undo history) and places the cursor.
    void applyRaw(const QString &raw, int cursorNibble);
    // Replaces the selection (or inserts at the cursor) with the hex digits of text, honouring maxBytes.
    void insertNibbles(const QString &text);
    void handleBackspace();
    void handleDelete();
    void reformatText();

    int maxBytes = 0xFFFF;
};

#endif
