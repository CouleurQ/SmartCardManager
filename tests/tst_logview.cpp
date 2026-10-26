#include "testregistry.h"
#include "ui/logview.h"

#include <QTest>
#include <QTreeView>
#include <QAbstractItemModel>

class TstLogView : public QObject
{
    Q_OBJECT

private slots:
    void addStringEntry();
    void addBytesEntryFormatsHexAndAscii();
    void nonCardEntriesHaveNoAscii();
    void typeColumnShowsTypeName();
    void iconPerType();
    void entriesAccumulateInOrder();
};

static QAbstractItemModel *modelOf(QTreeView &view) { return view.model(); }

void TstLogView::addStringEntry()
{
    QTreeView view;
    LogView log(&view);

    log.addLogEntry(LogType::Info, QString("hello"));

    QCOMPARE(modelOf(view)->rowCount(), 1);
    QCOMPARE(modelOf(view)->columnCount(), 3);
    QCOMPARE(modelOf(view)->index(0, 1).data().toString(), QString("hello"));
}

void TstLogView::addBytesEntryFormatsHexAndAscii()
{
    QTreeView view;
    LogView log(&view);

    log.addLogEntry(LogType::CardCmd, QByteArray::fromHex("00A4040041FF"));

    QCOMPARE(modelOf(view)->index(0, 1).data().toString(), QString("00 A4 04 00 41 FF"));
    // printable bytes are shown, everything else (incl. >= 0x80) becomes '.'
    QCOMPARE(modelOf(view)->index(0, 2).data().toString(), QString("....A."));
}

void TstLogView::nonCardEntriesHaveNoAscii()
{
    QTreeView view;
    LogView log(&view);

    log.addLogEntry(LogType::Error, QString("boom"));

    QVERIFY(modelOf(view)->index(0, 2).data().toString().isEmpty());
}

// BUG: nameForType() looks up an enumerator on LogView's meta object, but LogType is a plain
// enum class without Q_ENUM/Q_NAMESPACE, so the "Type" column text is always empty.
void TstLogView::typeColumnShowsTypeName()
{
    QTreeView view;
    LogView log(&view);

    log.addLogEntry(LogType::Warning, QString("careful"));

    QEXPECT_FAIL("", "Type column is empty because LogType is not a registered enum, CODE_REVIEW B-09", Continue);
    QCOMPARE(modelOf(view)->index(0, 0).data().toString(), QString("Warning"));
}

void TstLogView::iconPerType()
{
    QTreeView view;
    LogView log(&view);

    for (LogType t : {LogType::Info, LogType::Warning, LogType::Error, LogType::Terminal,
                      LogType::CardCmd, LogType::CardRsp})
        log.addLogEntry(t, QString("x"));

    QCOMPARE(modelOf(view)->rowCount(), 6);
    for (int row = 0; row < 6; ++row) {
        const QIcon icon = modelOf(view)->index(row, 0).data(Qt::DecorationRole).value<QIcon>();
        QVERIFY2(!icon.isNull(), qPrintable(QString("row %1 has no icon").arg(row)));
    }
}

void TstLogView::entriesAccumulateInOrder()
{
    QTreeView view;
    LogView log(&view);

    log.addLogEntry(LogType::Info, QString("one"));
    log.addLogEntry(LogType::Info, QString("two"));
    log.addLogEntry(LogType::Info, QString("three"));

    QCOMPARE(modelOf(view)->rowCount(), 3);
    QCOMPARE(modelOf(view)->index(2, 1).data().toString(), QString("three"));
}

static TestRegistrar<TstLogView> registrar;
#include "tst_logview.moc"
