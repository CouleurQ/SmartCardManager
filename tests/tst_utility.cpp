#include "testregistry.h"
#include "utility.h"

#include <QTest>

class TstUtility : public QObject
{
    Q_OBJECT

private slots:
    void hexStringToByteArray_data();
    void hexStringToByteArray();
    void hexStringToByteArray_oddDigitCountPadsTrailingZero();
};

void TstUtility::hexStringToByteArray_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("empty") << QString() << QByteArray();
    QTest::newRow("spaced") << "00 A4 04 00" << QByteArray::fromHex("00A40400");
    QTest::newRow("compact") << "00A40400" << QByteArray::fromHex("00A40400");
    QTest::newRow("lowercase") << "a4 0b" << QByteArray::fromHex("A40B");
    QTest::newRow("mixed separators") << "00:A4-04,00" << QByteArray::fromHex("00A40400");
    QTest::newRow("newlines and tabs") << "00 A4\n04\t00\r\n" << QByteArray::fromHex("00A40400");
    QTest::newRow("invalid chars are dropped") << "zz 00 gg A4" << QByteArray::fromHex("00A4");
    QTest::newRow("only invalid chars") << "xyz!?" << QByteArray();
}

void TstUtility::hexStringToByteArray()
{
    QFETCH(QString, input);
    QFETCH(QByteArray, expected);

    QCOMPARE(Utility::hexStringToByteArray(input), expected);
}

// Documents current behaviour: "A" becomes 0xA0. HexEdit::data() treats a lone nibble
// differently (see tst_hexedit::dataWithOddNibble) - see docs/CODE_REVIEW.md.
void TstUtility::hexStringToByteArray_oddDigitCountPadsTrailingZero()
{
    QCOMPARE(Utility::hexStringToByteArray("A"), QByteArray::fromHex("A0"));
    QCOMPARE(Utility::hexStringToByteArray("00 A"), QByteArray::fromHex("00A0"));
}

static TestRegistrar<TstUtility> registrar;
#include "tst_utility.moc"
