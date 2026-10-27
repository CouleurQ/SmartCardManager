#ifndef JAVACARDVIEW_H
#define JAVACARDVIEW_H

#include <QWidget>

#include "card/cardsession.h"
#include "card/gpauthparams.h"
#include "card/gpregistry.h"

class GpAuthDialog;
class GpSendPanel;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QToolButton;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;


// Java Card tab: GlobalPlatform registry (GET STATUS), card data (GET DATA) and keys of the card,
// plus the card content management actions.
// The GlobalPlatform commands are not implemented yet: authenticating shows simulated content.
class JavaCardView : public QWidget
{
    Q_OBJECT

public:
    explicit JavaCardView(CardSession *session, QWidget *parent = nullptr);

    bool isAuthenticated() const { return authenticated; }
    int workingChannel() const { return channel; }
    QString secureChannelText() const;

    // a card that goes away ends the secure channel
    void setConnected(bool connected);

    // shows the content of a card for the given authentication
    void showCard(const GpAuthParams &auth, const GpCardContent &content);
    void reset();

signals:
    void logBytes(LogType type, const QByteArray &bytes);
    void logString(LogType type, const QString &text);
    void stateChanged();

private:
    QWidget *makePage(const QString &title, QWidget *content);
    void showActions();
    void applyAuthentication();
    void sendCommand(const QByteArray &command, GpSendPanel *panel);
    void onResponseReceived(const QByteArray &response);

    void authenticate();
    void refresh();
    void notImplemented(const QString &action);
    void status(const QString &text);

    void openLogicalChannel();
    void closeLogicalChannel();
    void selectChannel(int channel);
    void showChannel();
    void updateChannelUi();
    QByteArray mapCommand(QByteArray command) const;
    void handleManageChannel(const QByteArray &response);

    void populateRegistry();
    void populateKeys();
    void showPlaceholders(const QString &text);
    void showEntryDetails();
    void openProperties(QTreeWidgetItem *item);
    void updatePropertyActions();
    void updateActions();
    void showSecureChannelDetails();
    void setDetailsVisible(bool visible);

    const GpRegistryEntry *currentEntry() const;
    QTreeWidgetItem *addEntry(QTreeWidgetItem *parent, int index);

    CardSession *session;
    QStackedWidget *stack;
    QWidget *actionsPage;
    GpAuthDialog *authPanel;
    GpSendPanel *apduPanel;
    GpSendPanel *bytesPanel;
    GpSendPanel *sendingPanel = nullptr;
    QPushButton *buttonSendApdu;
    QPushButton *buttonSendBytes;

    // state of the working channel; the other channels keep theirs in channels[]
    GpAuthParams auth;
    GpCardContent content;
    bool authenticated = false;
    bool connected = false;

    struct ChannelState {
        bool open = false;
        bool authenticated = false;
        GpAuthParams auth;
        GpCardContent content;
    };
    enum class PendingChannelOp { None, Open, Close };
    ChannelState channels[4];
    int channel = 0;
    PendingChannelOp pendingOp = PendingChannelOp::None;
    int pendingChannel = 0;
    QPushButton *buttonOpenChannel;
    QPushButton *buttonCloseChannel;
    QComboBox *channelCombo;

    QPushButton *buttonAuthenticate;
    QPushButton *buttonRefresh;
    QPushButton *buttonLoad;
    QPushButton *buttonInstall;
    QPushButton *buttonDelete;
    QPushButton *buttonSetStatus;
    QPushButton *buttonPutKey;
    QPushButton *buttonDeleteKey;

    QTreeWidget *registryTree;
    QTreeWidget *detailsTree;
    QWidget *propertiesPage;
    QPushButton *buttonPropSetStatus;
    QPushButton *buttonPropDelete;
    QPushButton *buttonPropInstall;
    QPushButton *buttonPropRefresh;
    QToolButton *detailsToggle;
    QPlainTextEdit *detailsText;
    QTreeWidget *keyTree;
};

#endif
