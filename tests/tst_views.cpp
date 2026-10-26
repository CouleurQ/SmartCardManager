#include "testregistry.h"
#include "hexedit.h"
#include "commands/commandviewapdu.h"
#include "commands/commandviewsendbytes.h"
#include "commands/commandviewterminal.h"

#include <QTest>
#include <QSignalSpy>
#include <QLineEdit>
#include <QComboBox>

class TstViews : public QObject
{
    Q_OBJECT

private slots:
    void apduShowsCommand();
    void apduEmptyForNullCommand();
    void apduEditsEmitUpdatedCommand();
    void apduLcFollowsDataLength();
    void apduFieldsAreLimited();
    void apduLcIsNotFocusable();
    void sendBytesShowsCommand();
    void sendBytesEditsEmitUpdatedCommand();
    void terminalShowsAction();
    void terminalNullCommandHasNoSelection();
    void terminalActionChangeEmitsCommand();
    void viewsGrabFocusOnNameField();
};

void TstViews::apduShowsCommand()
{
    auto cmd = QSharedPointer<CommandApdu>::create();
    cmd->commandName = "Select ISD";
    cmd->cla = QByteArray::fromHex("00");
    cmd->ins = QByteArray::fromHex("A4");
    cmd->p1 = QByteArray::fromHex("04");
    cmd->p2 = QByteArray::fromHex("00");
    cmd->commandData = QByteArray::fromHex("A000000151");
    cmd->le = QByteArray::fromHex("00");
    cmd->expectedResponse = QByteArray::fromHex("9000");

    CommandViewApdu view(cmd);

    QCOMPARE(view.findChild<QLineEdit *>("lineEditCommandName")->text(), QString("Select ISD"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditCLA")->toPlainText(), QString("00"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditINS")->toPlainText(), QString("A4"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditP1")->toPlainText(), QString("04"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditP2")->toPlainText(), QString("00"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditData")->toPlainText(), QString("A0 00 00 01 51"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditLe")->toPlainText(), QString("00"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditExpectedResponse")->toPlainText(), QString("90 00"));
    QCOMPARE(view.findChild<QLineEdit *>("lineEditLc")->text(), QString("5"));
}

void TstViews::apduEmptyForNullCommand()
{
    CommandViewApdu view;
    QVERIFY(view.findChild<HexEdit *>("hexEditCLA")->toPlainText().isEmpty());
    QVERIFY(view.findChild<HexEdit *>("hexEditData")->toPlainText().isEmpty());
}

void TstViews::apduEditsEmitUpdatedCommand()
{
    CommandViewApdu view;
    view.show();
    QSignalSpy updates(&view, &CommandView::notifyUpdateCommand);

    view.findChild<QLineEdit *>("lineEditCommandName")->setText("My command");
    QCOMPARE(updates.count(), 1);

    auto *cla = view.findChild<HexEdit *>("hexEditCLA");
    cla->setFocus();
    QTest::keyClicks(cla, "80");
    auto *ins = view.findChild<HexEdit *>("hexEditINS");
    ins->setFocus();
    QTest::keyClicks(ins, "CA");

    QVERIFY(updates.count() > 1);
    auto last = updates.last().at(0).value<QSharedPointer<Command>>();
    QCOMPARE(last->typeName(), QString("SendAPDU"));
    auto apdu = qSharedPointerDynamicCast<CommandApdu>(last);
    QVERIFY(apdu);
    QCOMPARE(apdu->commandName, QString("My command"));
    QCOMPARE(apdu->cla, QByteArray::fromHex("80"));
    QCOMPARE(apdu->ins, QByteArray::fromHex("CA"));
}

void TstViews::apduLcFollowsDataLength()
{
    CommandViewApdu view;
    view.show();
    auto *data = view.findChild<HexEdit *>("hexEditData");
    data->setFocus();
    QTest::keyClicks(data, "A0000001510000");   // 7 bytes
    QCOMPARE(view.findChild<QLineEdit *>("lineEditLc")->text(), QString("7"));
}

void TstViews::apduFieldsAreLimited()
{
    CommandViewApdu view;
    view.show();
    for (const char *name : {"hexEditCLA", "hexEditINS", "hexEditP1", "hexEditP2", "hexEditLe"}) {
        auto *edit = view.findChild<HexEdit *>(name);
        edit->setFocus();
        QTest::keyClicks(edit, "112233");
        QCOMPARE(edit->length(), 1);
    }
}

void TstViews::apduLcIsNotFocusable()
{
    CommandViewApdu view;
    QCOMPARE(view.findChild<QLineEdit *>("lineEditLc")->focusPolicy(), Qt::NoFocus);
}

void TstViews::sendBytesShowsCommand()
{
    auto cmd = QSharedPointer<CommandSendBytes>::create();
    cmd->commandName = "raw";
    cmd->commandData = QByteArray::fromHex("80CA9F7F00");
    cmd->expectedResponse = QByteArray::fromHex("9000");

    CommandViewSendBytes view(cmd);

    QCOMPARE(view.findChild<QLineEdit *>("lineEditCommandName")->text(), QString("raw"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditCommandApdu")->toPlainText(), QString("80 CA 9F 7F 00"));
    QCOMPARE(view.findChild<HexEdit *>("hexEditExpectedResponse")->toPlainText(), QString("90 00"));
}

void TstViews::sendBytesEditsEmitUpdatedCommand()
{
    CommandViewSendBytes view;
    view.show();
    QSignalSpy updates(&view, &CommandView::notifyUpdateCommand);

    view.findChild<QLineEdit *>("lineEditCommandName")->setText("raw");
    auto *edit = view.findChild<HexEdit *>("hexEditCommandApdu");
    edit->setFocus();
    QTest::keyClicks(edit, "00A40400");

    QVERIFY(updates.count() > 1);
    auto cmd = qSharedPointerDynamicCast<CommandSendBytes>(updates.last().at(0).value<QSharedPointer<Command>>());
    QVERIFY(cmd);
    QCOMPARE(cmd->commandName, QString("raw"));
    QCOMPARE(cmd->commandData, QByteArray::fromHex("00A40400"));
}

void TstViews::terminalShowsAction()
{
    auto cmd = QSharedPointer<CommandTerminal>::create();
    cmd->commandName = "go";
    cmd->action = CommandTerminal::ActionType::Connect;

    CommandViewTerminal view(cmd);

    QCOMPARE(view.findChild<QLineEdit *>("lineEditCommandName")->text(), QString("go"));
    auto *combo = view.findChild<QComboBox *>("comboBoxAction");
    QCOMPARE(combo->count(), 6);
    QCOMPARE(combo->currentText(), QString("Connect"));
}

void TstViews::terminalNullCommandHasNoSelection()
{
    CommandViewTerminal view;
    QCOMPARE(view.findChild<QComboBox *>("comboBoxAction")->currentIndex(), -1);
}

void TstViews::terminalActionChangeEmitsCommand()
{
    CommandViewTerminal view;
    QSignalSpy updates(&view, &CommandView::notifyUpdateCommand);

    view.findChild<QComboBox *>("comboBoxAction")->setCurrentIndex(3);   // Wait for removal

    QCOMPARE(updates.count(), 1);
    auto cmd = qSharedPointerDynamicCast<CommandTerminal>(updates.first().at(0).value<QSharedPointer<Command>>());
    QVERIFY(cmd);
    QCOMPARE(cmd->action, CommandTerminal::ActionType::WaitForRemoval);
}

void TstViews::viewsGrabFocusOnNameField()
{
    CommandViewApdu view;
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    auto *name = view.findChild<QLineEdit *>("lineEditCommandName");
    QTRY_VERIFY(name->hasFocus());
}

static TestRegistrar<TstViews> registrar;
#include "tst_views.moc"
