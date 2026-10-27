#ifndef GPSENDPANEL_H
#define GPSENDPANEL_H

#include <QWidget>

class HexEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTextEdit;


// Edit and send one command: either as APDU fields (CLA INS P1 P2 Lc data Le) or as raw bytes,
// with the status word and the response below (like the Send Command tab).
class GpSendPanel : public QWidget
{
    Q_OBJECT

public:
    enum class Mode { Apdu, Bytes };

    explicit GpSendPanel(Mode mode, QWidget *parent = nullptr);

    QByteArray command() const;
    void setSendEnabled(bool enabled);
    void showResponse(const QByteArray &response);
    void clearResponse();

signals:
    void sendRequested(const QByteArray &command);

private:
    void updatePreview();
    void send();

    Mode mode;
    HexEdit *cla = nullptr;
    HexEdit *ins = nullptr;
    HexEdit *p1 = nullptr;
    HexEdit *p2 = nullptr;
    HexEdit *data = nullptr;
    HexEdit *le = nullptr;
    QLineEdit *lc = nullptr;
    HexEdit *rawBytes = nullptr;
    QPushButton *buttonSend;
    QLineEdit *sw;
    QLineEdit *swText;
    QTextEdit *response;
};

#endif
