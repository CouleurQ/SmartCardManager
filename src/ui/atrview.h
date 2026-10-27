#ifndef ATRVIEW_H
#define ATRVIEW_H

#include <QWidget>

class QLabel;
class QTreeWidget;


// Shows an ATR, a short summary and the byte by byte interpretation; selecting a line of the
// interpretation marks the corresponding bytes of the ATR.
class AtrView : public QWidget
{
    Q_OBJECT

public:
    explicit AtrView(QWidget *parent = nullptr);

    void setAtr(const QByteArray &atr);
    QByteArray atr() const { return currentAtr; }

private:
    // marks bytes [first, first+count) of the ATR string; first < 0 clears the marking
    void highlight(int first, int count);

    QByteArray currentAtr;
    QLabel *atrLabel;
    QLabel *summaryLabel;
    QLabel *warningLabel;
    QTreeWidget *tree;
};

#endif
