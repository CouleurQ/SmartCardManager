#include "testregistry.h"
#include "card/atrparser.h"
#include "ui/atrview.h"

#include <QLabel>
#include <QTest>
#include <QTreeWidget>

using namespace AtrParser;

class TstAtr : public QObject
{
    Q_OBJECT

private slots:
    void emptyAndShortAtrsAreRejected();
    void contactAtrWithStatusInformation();
    void contactlessAtrIsRecognised();
    void ta1SelectsSpeedParameters();
    void invalidTckIsReported();
    void missingTckIsReported();
    void truncatedHistoricalBytes();
    void interfaceBytesOfT1();
    void viewShowsAtrAndRows();
    void selectingARowHighlightsItsBytes();
};

static const Row *find(const QList<Row> &rows, const QString &field)
{
    for (const Row &row : rows) {
        if (row.field.startsWith(field))
            return &row;
        if (const Row *child = find(row.children, field))
            return child;
    }
    return nullptr;
}

static bool summaryContains(const Info &info, const QString &text)
{
    return info.summary.join('\n').contains(text);
}

void TstAtr::emptyAndShortAtrsAreRejected()
{
    QVERIFY(!parse(QByteArray()).valid);
    QVERIFY(!parse(QByteArray()).warnings.isEmpty());
    QVERIFY(!parse(QByteArray::fromHex("3B")).valid);
}

void TstAtr::contactAtrWithStatusInformation()
{
    const Info info = parse(QByteArray::fromHex("3B6800000073C84013009000"));
    QVERIFY(info.valid);
    QVERIFY(info.warnings.isEmpty());
    QVERIFY(summaryContains(info, "Direct convention"));
    QVERIFY(summaryContains(info, "T=0"));
    QVERIFY(find(info.rows, "TB1"));
    QVERIFY(find(info.rows, "TC1"));
    QVERIFY(!find(info.rows, "TCK"));
    const Row *caps = find(info.rows, "Card capabilities");
    QVERIFY(caps);
    QVERIFY(caps->meaning.contains("DF selection by full name"));
    QVERIFY(find(info.rows, "Status information"));
    QCOMPARE(info.byteKinds.size(), 12);
    QCOMPARE(info.byteKinds[0], Kind::Ts);
    QCOMPARE(info.byteKinds[1], Kind::T0);
    QCOMPARE(info.byteKinds[2], Kind::Interface);
    QCOMPARE(info.byteKinds[4], Kind::Historical);
}

void TstAtr::contactlessAtrIsRecognised()
{
    const Info info = parse(QByteArray::fromHex("3B8F8001804F0CA00000030603000100000000"
        "6A"));
    QVERIFY(info.valid);
    QVERIFY(info.warnings.isEmpty());
    QVERIFY(summaryContains(info, "MIFARE Classic 1K"));
    QVERIFY(summaryContains(info, "ISO 14443 A, part 3"));
    QVERIFY(summaryContains(info, "T=0, T=1"));
    const Row *tck = find(info.rows, "TCK");
    QVERIFY(tck);
    QVERIFY(tck->meaning.contains("valid"));
    QVERIFY(find(info.rows, "Card name")->meaning.contains("MIFARE Classic 1K"));
}

void TstAtr::ta1SelectsSpeedParameters()
{
    const Info info = parse(QByteArray::fromHex("3B1096"));
    QVERIFY(info.valid);
    QVERIFY(summaryContains(info, "Fi = 512"));
    QVERIFY(summaryContains(info, "Di = 32"));

    const Info defaults = parse(QByteArray::fromHex("3B00"));
    QVERIFY(summaryContains(defaults, "Fi = 372"));
}

void TstAtr::invalidTckIsReported()
{
    const Info info = parse(QByteArray::fromHex("3B8F8001804F0CA00000030603000100000000"
        "6B"));
    QVERIFY(info.warnings.join('\n').contains("TCK"));
    QVERIFY(find(info.rows, "TCK")->meaning.contains("INVALID"));
}

void TstAtr::missingTckIsReported()
{
    const Info info = parse(QByteArray::fromHex("3B8001"));   // T=1 offered, no TCK
    QVERIFY(!info.valid);
    QVERIFY(info.warnings.join('\n').contains("TCK is missing"));
}

void TstAtr::truncatedHistoricalBytes()
{
    const Info info = parse(QByteArray::fromHex("3B05414243"));   // 5 announced, 3 present
    QVERIFY(!info.valid);
    QVERIFY(!info.warnings.isEmpty());
}

void TstAtr::interfaceBytesOfT1()
{
    // TD1 = 81 (T=1, TD2 follows), TD2 = 31 (T=1, TA3 TB3), TA3 = IFSC FE, TB3 = BWI 4 / CWI 5; TCK
    QByteArray atr = QByteArray::fromHex("3B8081" "31" "FE45");
    quint8 x = 0;
    for (int i = 1; i < atr.size(); ++i)
        x ^= static_cast<quint8>(atr[i]);
    atr.append(static_cast<char>(x));

    const Info info = parse(atr);
    QVERIFY(info.valid);
    QVERIFY(info.warnings.isEmpty());
    QVERIFY(summaryContains(info, "IFSC: 254"));
    QVERIFY(find(info.rows, "TB3")->meaning.contains("BWI = 4"));
    QVERIFY(find(info.rows, "TB3")->meaning.contains("CWI = 5"));
}

void TstAtr::viewShowsAtrAndRows()
{
    AtrView view;
    QVERIFY(view.findChild<QLabel *>("atrLabel")->text().contains("No ATR"));

    view.setAtr(QByteArray::fromHex("3B6800000073C84013009000"));
    QVERIFY(view.findChild<QLabel *>("atrLabel")->text().contains("73"));
    QVERIFY(view.findChild<QTreeWidget *>("atrTree")->topLevelItemCount() >= 3);
    QVERIFY(view.findChild<QLabel *>("atrSummary")->text().contains("T=0"));

    view.setAtr(QByteArray());
    QCOMPARE(view.findChild<QTreeWidget *>("atrTree")->topLevelItemCount(), 0);
}

void TstAtr::selectingARowHighlightsItsBytes()
{
    AtrView view;
    view.setAtr(QByteArray::fromHex("3B6800000073C84013009000"));
    auto *label = view.findChild<QLabel *>("atrLabel");
    auto *tree = view.findChild<QTreeWidget *>("atrTree");
    QVERIFY(!label->text().contains("background-color"));
    QVERIFY(!label->text().contains("color:#"));
    QVERIFY(view.findChild<QLabel *>("atrLegend") == nullptr);

    tree->setCurrentItem(tree->topLevelItem(1));     // T0
    QCOMPARE(label->text().count("background-color"), 1);
    QVERIFY(label->text().contains(">68</span>"));

    tree->setCurrentItem(tree->topLevelItem(3));     // historical bytes (8)
    QCOMPARE(label->text().count("background-color"), 8);
}

static TestRegistrar<TstAtr> registrar;
#include "tst_atr.moc"
