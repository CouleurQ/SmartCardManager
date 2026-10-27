#include "testregistry.h"
#include "mocktransport.h"
#include "card/cardsession.h"
#include "commands/commandapdu.h"
#include "hexedit.h"
#include "ui/gpsendpanel.h"
#include <QLineEdit>
#include <QSignalSpy>
#include <QStackedWidget>
#include "card/gpcoding.h"
#include "card/gpdemodata.h"
#include "ui/gpauthdialog.h"
#include "ui/javacardview.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTest>
#include <QToolButton>
#include <QTreeWidget>

class TstJavaCard : public QObject
{
    Q_OBJECT

private:
    MockTransport *transport = nullptr;
    CardSession *session = nullptr;

private slots:
    void init() { transport = new MockTransport; session = new CardSession(transport); }
    void cleanup() { delete session; delete transport; session = nullptr; transport = nullptr; }

    void lifeCycleCodings();
    void privilegeCodings();
    void keyAndSecurityLevelCodings();
    void unknownValuesAreReported();
    void demoContentFollowsAuthentication();
    void dialogRoundTripsParameters();
    void dialogSecurityLevelFromCheckBoxes();
    void viewStartsWithPlaceholders();
    void viewShowsRegistryDataAndKeys();
    void viewResetsWhenCardGoesAway();
    void actionsAreNotImplemented();
    void secureChannelDetailsAreCollapsible();
    void openSecureChannelSwitchesTheMiddlePanel();
    void sendApduShowsStatusWordAndResponse();
    void sendBytesNeedsAConnection();
    void logicalChannels();
    void dialogHexFieldsLimitTheirSize();
};

static QTreeWidgetItem *findItem(QTreeWidget *tree, const QString &text)
{
    const auto items = tree->findItems(text, Qt::MatchContains | Qt::MatchRecursive, 0);
    return items.isEmpty() ? nullptr : items.first();
}

void TstJavaCard::lifeCycleCodings()
{
    using GpCoding::ObjectKind;
    QCOMPARE(GpCoding::lifeCycleText(ObjectKind::Card, 0x0F), QString("SECURED"));
    QCOMPARE(GpCoding::lifeCycleText(ObjectKind::Card, 0x7F), QString("CARD_LOCKED"));
    QCOMPARE(GpCoding::lifeCycleText(ObjectKind::ExecutableLoadFile, 0x01), QString("LOADED"));
    QCOMPARE(GpCoding::lifeCycleText(ObjectKind::Application, 0x07), QString("SELECTABLE"));
    QCOMPARE(GpCoding::lifeCycleText(ObjectKind::Application, 0x83), QString("LOCKED"));
    QCOMPARE(GpCoding::lifeCycleText(ObjectKind::SecurityDomain, 0x0F), QString("PERSONALIZED"));
}

void TstJavaCard::privilegeCodings()
{
    const QStringList isd = GpCoding::privilegeNames(QByteArray::fromHex("9E7C80"));
    QVERIFY(isd.contains("Security Domain"));
    QVERIFY(GpCoding::privilegeNames(QByteArray::fromHex("000000")).isEmpty());
}

void TstJavaCard::keyAndSecurityLevelCodings()
{
    QVERIFY(GpCoding::keyTypeText(0x88).contains("AES"));
    QVERIFY(GpCoding::keyTypeText(0x80).contains("DES"));
    QVERIFY(GpCoding::securityLevelText(0x03).contains("C_MAC"));
    QVERIFY(GpCoding::securityLevelText(0x03).contains("C_DECRYPTION"));
}

void TstJavaCard::unknownValuesAreReported()
{
    QVERIFY(GpCoding::keyTypeText(0xEE).contains("Unknown"));
    QVERIFY(GpCoding::lifeCycleText(GpCoding::ObjectKind::Card, 0x55).contains("Unknown"));
}

void TstJavaCard::demoContentFollowsAuthentication()
{
    GpAuthParams params;
    params.securityLevel = 0x03;
    const GpCardContent content = GpDemoData::content(params);
    QVERIFY(!content.registry.isEmpty());
    QVERIFY(!content.keys.isEmpty());

    QByteArray level;
    for (const GpDataObject &o : content.dataObjects) {
        if (o.tag == QByteArray::fromHex("D3"))
            level = o.value;
    }
    QCOMPARE(level, QByteArray::fromHex("83"));

    int isd = 0;
    for (const GpRegistryEntry &e : content.registry)
        isd += e.kind == GpRegistryEntry::Kind::IssuerSecurityDomain;
    QCOMPARE(isd, 1);
}

void TstJavaCard::dialogRoundTripsParameters()
{
    GpAuthParams in;
    in.protocol = GpAuthParams::Protocol::Scp02;
    in.securityLevel = 0x01;
    in.keyVersion = 0x20;
    in.keyId = 0x01;
    in.selectFirst = true;
    in.targetAid = QByteArray::fromHex("A000000151535041");

    GpAuthDialog dialog;
    dialog.setParams(in);
    const GpAuthParams out = dialog.params();
    QCOMPARE(out.protocol, in.protocol);
    QCOMPARE(out.securityLevel, in.securityLevel);
    QCOMPARE(out.keyVersion, in.keyVersion);
    QCOMPARE(out.targetAid, in.targetAid);
}

void TstJavaCard::dialogHexFieldsLimitTheirSize()
{
    GpAuthDialog dialog;
    auto *aid = dialog.findChild<QComboBox *>("targetAid")->lineEdit();
    aid->clear();
    QTest::keyClicks(aid, "a0000001515350415041");
    QCOMPARE(aid->text(), QString("A0 00 00 01 51 53 50 41 50 41"));
    aid->clear();
    QTest::keyClicks(aid, "A000000151535041504142434445464748");     // more than 16 bytes
    QCOMPARE(aid->text().remove(' ').size(), 32);
    aid->clear();
    QTest::keyClicks(aid, "xyz");
    QVERIFY(aid->text().isEmpty());

    dialog.findChild<QRadioButton *>("challengeFixed")->setChecked(true);
    auto *challenge = dialog.findChild<QLineEdit *>("hostChallenge");
    QTest::keyClicks(challenge, "0102030405060708090A");
    QCOMPARE(challenge->text(), QString("01 02 03 04 05 06 07 08"));
}


void TstJavaCard::dialogSecurityLevelFromCheckBoxes()
{
    GpAuthDialog dialog;
    dialog.setParams(GpAuthParams());
    auto *cmac = dialog.findChild<QCheckBox *>("levelCMac");
    auto *cdec = dialog.findChild<QCheckBox *>("levelCDec");
    QVERIFY(cmac && cdec);

    cdec->setChecked(true);     // C-DECRYPTION requires C-MAC
    QVERIFY(cmac->isChecked());
    QCOMPARE(dialog.params().securityLevel, quint8(0x03));

    cmac->setChecked(false);
    QVERIFY(!cdec->isChecked());
    QCOMPARE(dialog.params().securityLevel, quint8(0x00));
}

void TstJavaCard::viewStartsWithPlaceholders()
{
    JavaCardView view(session);
    QVERIFY(!view.isAuthenticated());
    QVERIFY(!view.findChild<QPushButton *>("buttonAuthenticate")->isEnabled());
    view.setConnected(true);
    QVERIFY(view.findChild<QPushButton *>("buttonAuthenticate")->isEnabled());
    view.setConnected(false);
    QVERIFY(!view.findChild<QPushButton *>("buttonAuthenticate")->isEnabled());
    QVERIFY(!view.findChild<QPushButton *>("buttonRefresh")->isEnabled());
    QVERIFY(!view.findChild<QPushButton *>("buttonLoad")->isEnabled());
    QVERIFY(!view.findChild<QPushButton *>("buttonPutKey")->isEnabled());
    QCOMPARE(view.findChild<QTreeWidget *>("registryTree")->topLevelItemCount(), 1);
}

void TstJavaCard::viewShowsRegistryDataAndKeys()
{
    JavaCardView view(session);
    GpAuthParams params;
    view.showCard(params, GpDemoData::content(params));
    QVERIFY(view.isAuthenticated());

    auto *registry = view.findChild<QTreeWidget *>("registryTree");
    auto *keys = view.findChild<QTreeWidget *>("keyTree");

    QTreeWidgetItem *isd = registry->topLevelItem(0);
    QCOMPARE(registry->columnCount(), 2);
    QCOMPARE(isd->text(0), QString("A000000151000000"));
    QCOMPARE(isd->text(1), QString("Issuer Security Domain"));
    QVERIFY(isd->childCount() > 0);
    QVERIFY(findItem(registry, "Executable Load Files"));
    QVERIFY(keys->topLevelItemCount() >= 2);

    // the ISD cannot be deleted, an application can
    auto *buttonDelete = view.findChild<QPushButton *>("buttonDelete");
    registry->setCurrentItem(isd);
    QVERIFY(!buttonDelete->isEnabled());
    registry->setCurrentItem(findItem(registry, "F0010203040501"));
    QVERIFY(buttonDelete->isEnabled());

    // double click opens the properties in the middle panel
    auto *stack = view.findChild<QStackedWidget *>("middleStack");
    emit registry->itemDoubleClicked(registry->currentItem(), 0);
    QVERIFY(stack->currentWidget() != view.findChild<QWidget *>("actionsPanel"));
    QVERIFY(view.findChild<QTreeWidget *>("detailsTree")->topLevelItemCount() > 3);
    QVERIFY(!view.findChild<QPushButton *>("buttonPropInstall")->isEnabled());
    QVERIFY(view.findChild<QPushButton *>("buttonPropDelete")->isEnabled());
    view.findChild<QPushButton *>("buttonBack")->click();
    QCOMPARE(stack->currentWidget(), view.findChild<QWidget *>("actionsPanel"));

    QVERIFY(!view.findChild<QTreeWidget *>("dataTree"));

    QVERIFY(view.findChild<QPushButton *>("buttonRefresh")->isEnabled());
}

void TstJavaCard::viewResetsWhenCardGoesAway()
{
    JavaCardView view(session);
    view.setConnected(true);
    GpAuthParams params;
    view.showCard(params, GpDemoData::content(params));
    QVERIFY(view.isAuthenticated());

    view.setConnected(false);
    QVERIFY(!view.isAuthenticated());
    QCOMPARE(view.findChild<QTreeWidget *>("registryTree")->topLevelItemCount(), 1);
}

void TstJavaCard::actionsAreNotImplemented()
{
    JavaCardView view(session);
    QSignalSpy spy(&view, &JavaCardView::logString);
    GpAuthParams params;
    view.showCard(params, GpDemoData::content(params));
    QCOMPARE(view.secureChannelText(), QString("SCP03"));
    view.findChild<QPushButton *>("buttonLoad")->click();
    QVERIFY(spy.last().at(1).toString().contains("not implemented"));
}

void TstJavaCard::secureChannelDetailsAreCollapsible()
{
    JavaCardView view(session);
    view.show();
    auto *toggle = view.findChild<QToolButton *>("detailsToggle");
    auto *details = view.findChild<QPlainTextEdit *>("secureChannelDetails");
    QVERIFY(!toggle->isEnabled());
    QVERIFY(!details->isVisible());

    GpAuthParams params;
    view.showCard(params, GpDemoData::content(params));
    QVERIFY(toggle->isEnabled());
    QVERIFY(!details->isVisible());
    toggle->click();
    QVERIFY(details->isVisible());
    QVERIFY(details->toPlainText().contains("S-ENC"));
    QVERIFY(details->toPlainText().contains("SCP03"));
    toggle->click();
    QVERIFY(!details->isVisible());
}

void TstJavaCard::openSecureChannelSwitchesTheMiddlePanel()
{
    JavaCardView view(session);
    view.show();
    QVERIFY(!view.findChild<QPushButton *>("buttonAuthenticate")->isEnabled());
    view.setConnected(true);
    auto *stack = view.findChild<QStackedWidget *>("middleStack");
    auto *actions = view.findChild<QWidget *>("actionsPanel");
    auto *dialog = view.findChild<GpAuthDialog *>("gpAuthDialog");
    QCOMPARE(stack->currentWidget(), actions);

    view.findChild<QPushButton *>("buttonAuthenticate")->click();
    QVERIFY(stack->currentWidget() != actions);
    QVERIFY(dialog->isVisible());

    dialog->reject();
    QCOMPARE(stack->currentWidget(), actions);
    QVERIFY(!view.isAuthenticated());

    view.findChild<QPushButton *>("buttonAuthenticate")->click();
    QVERIFY(dialog->isVisible());
    dialog->accept();
    QCOMPARE(stack->currentWidget(), actions);
    QVERIFY(view.isAuthenticated());
    QVERIFY(view.findChild<QTreeWidget *>("registryTree")->topLevelItemCount() > 1);
    QVERIFY(view.findChild<QToolButton *>("detailsToggle")->text().contains("A0 00 00 01 51 00 00 00"));
}

void TstJavaCard::sendApduShowsStatusWordAndResponse()
{
    session->setReader("Mock Reader 0");
    QVERIFY(session->connectCard());
    transport->answers[QByteArray::fromHex("00A4040000")] = QByteArray::fromHex("6F0084009000");

    JavaCardView view(session);
    view.show();
    view.setConnected(true);
    QSignalSpy spy(&view, &JavaCardView::logBytes);

    auto *button = view.findChild<QPushButton *>("buttonSendApdu");
    QVERIFY(button->isEnabled());
    button->click();
    auto *panel = view.findChild<GpSendPanel *>("apduPanel");
    panel->findChild<HexEdit *>("hexIns")->setData(QByteArray::fromHex("A4"));
    panel->findChild<HexEdit *>("hexP1")->setData(QByteArray::fromHex("04"));
    panel->findChild<HexEdit *>("hexLe")->setData(QByteArray::fromHex("00"));
    QCOMPARE(panel->command(), QByteArray::fromHex("00A4040000"));
    QVERIFY(!panel->findChild<QLineEdit *>("commandPreview"));

    panel->findChild<QPushButton *>("buttonSend")->click();
    QTRY_COMPARE(panel->findChild<QLineEdit *>("lineSw")->text(), QString("90 00"));
    QCOMPARE(spy.count(), 2);       // command and response
    QCOMPARE(transport->sent.last(), QByteArray::fromHex("00A4040000"));
}

void TstJavaCard::sendBytesNeedsAConnection()
{
    JavaCardView view(session);
    QVERIFY(!view.findChild<QPushButton *>("buttonSendBytes")->isEnabled());
    view.setConnected(true);
    QVERIFY(view.findChild<QPushButton *>("buttonSendBytes")->isEnabled());

    auto *panel = view.findChild<GpSendPanel *>("bytesPanel");
    auto *send = panel->findChild<QPushButton *>("buttonSend");
    QVERIFY(!send->isEnabled());            // nothing to send yet
    panel->findChild<HexEdit *>("hexBytes")->setData(QByteArray::fromHex("00CA006600"));
    QVERIFY(send->isEnabled());
    view.setConnected(false);
    QVERIFY(!send->isEnabled());
}

static TestRegistrar<TstJavaCard> registrar;
#include "tst_javacard.moc"


void TstJavaCard::logicalChannels()
{
    session->setReader("Mock Reader 0");
    QVERIFY(session->connectCard());
    transport->answers[QByteArray::fromHex("0070000001")] = QByteArray::fromHex("019000");

    JavaCardView view(session);
    view.show();
    auto *open = view.findChild<QPushButton *>("buttonOpenChannel");
    auto *close = view.findChild<QPushButton *>("buttonCloseChannel");
    auto *combo = view.findChild<QComboBox *>("channelCombo");
    QVERIFY(!open->isEnabled());
    view.setConnected(true);
    QVERIFY(open->isEnabled());
    QVERIFY(!close->isEnabled());
    QVERIFY(combo->currentText().contains("0 - Basic channel"));

    open->click();
    QTRY_VERIFY(combo->currentText().contains("1 - Logical channel"));
    QVERIFY(close->isEnabled());
    QVERIFY(!view.isAuthenticated());       // every channel has its own secure channel

    // commands go out on the working channel
    auto *panel = view.findChild<GpSendPanel *>("bytesPanel");
    panel->findChild<HexEdit *>("hexBytes")->setData(QByteArray::fromHex("80CA006600"));
    panel->findChild<QPushButton *>("buttonSend")->click();
    QTRY_COMPARE(transport->sent.last(), QByteArray::fromHex("81CA006600"));

    transport->answers[QByteArray::fromHex("00708001")] = QByteArray::fromHex("9000");
    close->click();
    QTRY_VERIFY(combo->currentText().contains("0 - Basic channel"));
    QVERIFY(!close->isEnabled());
}
