#include "gpauthdialog.h"

#include "card/gpcoding.h"
#include "utility.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpressionValidator>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

using Protocol = GpAuthParams::Protocol;

const QString AutoText = "Auto";

QString cleanHex(const QString &text)
{
    QString digits;
    for (const QChar c : text) {
        if (c.isDigit() || (c.toUpper() >= 'A' && c.toUpper() <= 'F'))
            digits += c;
    }
    return digits;
}


QString hex2(int value)
{
    return QString("%1").arg(value, 2, 16, QChar('0')).toUpper();
}


// Hex digits only, at most maxBytes bytes, shown upper case in groups of two ("A0 00 00 01")
class HexBytesValidator : public QValidator
{
public:
    HexBytesValidator(int maxBytes, QObject *parent) : QValidator(parent), limit(maxBytes) {}

    State validate(QString &input, int &pos) const override
    {
        QString digits;
        int digitsBefore = 0;
        for (int i = 0; i < input.size(); ++i) {
            const QChar c = input.at(i);
            if (c == ' ')
                continue;
            if (!c.isDigit() && !(c.toUpper() >= 'A' && c.toUpper() <= 'F'))
                return Invalid;
            digits += c.toUpper();
            if (i < pos)
                ++digitsBefore;
        }
        if (digits.size() > 2 * limit)
            return Invalid;

        QString text;
        for (int i = 0; i < digits.size(); ++i) {
            if (i > 0 && i % 2 == 0)
                text += ' ';
            text += digits.at(i);
        }
        input = text;
        pos = qMin(digitsBefore + digitsBefore / 2, int(text.size()));
        return Acceptable;
    }

private:
    int limit;
};


QLineEdit *hexLineEdit(QWidget *parent, const QString &objectName)
{
    auto *edit = new QLineEdit(parent);
    edit->setObjectName(objectName);
    edit->setValidator(new HexBytesValidator(32, edit));
    edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    return edit;
}


QStringList iValues(Protocol p)
{
    switch (p) {
    case Protocol::Scp01: return {"05", "15"};
    case Protocol::Scp02: return {"15", "55", "1A"};
    case Protocol::Scp03: return {"00", "10", "20", "30", "60", "70"};
    default: return {};
    }
}

} // namespace


GpAuthDialog::GpAuthDialog(QWidget *parent)
    : QDialog(parent)
{
    setObjectName("gpAuthDialog");
    setWindowTitle("Authenticate - GlobalPlatform Secure Channel");

    readRegistry = new QCheckBox("Read registry and keys after authentication");
    readRegistry->setObjectName("readRegistry");
    readRegistry->setToolTip("GET STATUS (registry), GET DATA (card data) and the key information");
    readRegistry->setChecked(true);

    auto *left = new QVBoxLayout;
    left->addWidget(buildDomainGroup());
    left->addStretch(1);
    left->addWidget(buildProtocolGroup());
    left->addStretch(1);
    left->addWidget(buildSecurityLevelGroup());

    auto *right = new QVBoxLayout;
    right->addWidget(buildKeysGroup());
    right->addWidget(buildScp11Group());

    auto *columns = new QHBoxLayout;
    columns->addLayout(left, 1);
    columns->addLayout(right, 1);

    problemLabel = new QLabel;
    problemLabel->setObjectName("problemLabel");
    problemLabel->setStyleSheet("color: #b00020;");

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText("Authenticate");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(columns);
    layout->addWidget(readRegistry);
    layout->addStretch();
    layout->addWidget(problemLabel);
    layout->addWidget(buttons);

    connect(selectFirst, &QCheckBox::toggled, targetAid, &QWidget::setEnabled);
    connect(selectFirst, &QCheckBox::toggled, this, &GpAuthDialog::refresh);
    connect(targetAid, &QComboBox::editTextChanged, this, &GpAuthDialog::refresh);
    connect(protocolCombo, &QComboBox::currentIndexChanged, this, &GpAuthDialog::protocolChanged);
    connect(iParameter, &QComboBox::editTextChanged, this, &GpAuthDialog::refresh);
    connect(challengeFixed, &QRadioButton::toggled, hostChallenge, &QWidget::setEnabled);
    connect(challengeFixed, &QRadioButton::toggled, this, &GpAuthDialog::refresh);
    connect(hostChallenge, &QLineEdit::textChanged, this, &GpAuthDialog::refresh);
    connect(keySource, &QComboBox::currentIndexChanged, this, &GpAuthDialog::keySourceChanged);
    for (QLineEdit *edit : {encKey, macKey, dekKey, masterKey, oceCertificate, oceKey})
        connect(edit, &QLineEdit::textChanged, this, &GpAuthDialog::refresh);
    connect(diversification, &QComboBox::currentIndexChanged, this, &GpAuthDialog::refresh);
    connect(keyVersion, &QSpinBox::valueChanged, this, &GpAuthDialog::refresh);
    connect(keyId, &QSpinBox::valueChanged, this, &GpAuthDialog::refresh);
    connect(showKeys, &QCheckBox::toggled, this, [this](bool show) {
        const auto mode = show ? QLineEdit::Normal : QLineEdit::Password;
        for (QLineEdit *edit : {encKey, macKey, dekKey, masterKey})
            edit->setEchoMode(mode);
    });

    // a checked decryption / encryption bit needs its MAC bit
    connect(levelCDec, &QCheckBox::toggled, this, [this](bool on) { if (on) levelCMac->setChecked(true); refresh(); });
    connect(levelCMac, &QCheckBox::toggled, this, [this](bool on) { if (!on) levelCDec->setChecked(false); refresh(); });
    connect(levelREnc, &QCheckBox::toggled, this, [this](bool on) { if (on) levelRMac->setChecked(true); refresh(); });
    connect(levelRMac, &QCheckBox::toggled, this, [this](bool on) { if (!on) levelREnc->setChecked(false); refresh(); });

    setParams(GpAuthParams());
}


QGroupBox *GpAuthDialog::buildDomainGroup()
{
    domainGroup = new QGroupBox("Security Domain");
    domainGroup->setObjectName("domainGroup");

    selectFirst = new QCheckBox("SELECT the Security Domain first");
    selectFirst->setObjectName("selectFirst");

    targetAid = new QComboBox;
    targetAid->setObjectName("targetAid");
    targetAid->setEditable(true);
    targetAid->addItems({"A0 00 00 01 51 00 00 00", "A0 00 00 01 51 53 50 41"});
    targetAid->setToolTip("AID of the Security Domain (5 to 16 bytes). A0 00 00 01 51 00 00 00 is the default Issuer Security Domain.");
    targetAid->lineEdit()->setValidator(new HexBytesValidator(16, targetAid));
    targetAid->lineEdit()->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    auto *layout = new QFormLayout(domainGroup);
    layout->addRow(selectFirst);
    layout->addRow("AID:", targetAid);
    return domainGroup;
}


QGroupBox *GpAuthDialog::buildProtocolGroup()
{
    auto *group = new QGroupBox("Secure Channel Protocol");

    protocolCombo = new QComboBox;
    protocolCombo->setObjectName("protocolCombo");
    protocolCombo->addItem("SCP03 - AES (Amendment D)", int(Protocol::Scp03));
    protocolCombo->addItem("SCP02 - 3DES (deprecated)", int(Protocol::Scp02));
    protocolCombo->addItem("SCP01 - 3DES (deprecated)", int(Protocol::Scp01));
    protocolCombo->addItem("SCP11a - ECC, mutual authentication (Amendment F)", int(Protocol::Scp11a));
    protocolCombo->addItem("SCP11b - ECC, card authentication (Amendment F)", int(Protocol::Scp11b));
    protocolCombo->addItem("SCP11c - ECC, mutual authentication, scripting (Amendment F)", int(Protocol::Scp11c));

    iParameter = new QComboBox;
    iParameter->setObjectName("iParameter");
    iParameter->setEditable(true);
    iParameter->lineEdit()->setValidator(
        new QRegularExpressionValidator(QRegularExpression("Auto|[0-9A-Fa-f]{0,2}"), iParameter));

    challengeRandom = new QRadioButton("Random");
    challengeRandom->setObjectName("challengeRandom");
    challengeRandom->setChecked(true);
    challengeFixed = new QRadioButton("Fixed");
    challengeFixed->setObjectName("challengeFixed");
    hostChallenge = hexLineEdit(group, "hostChallenge");
    hostChallenge->setValidator(new HexBytesValidator(8, hostChallenge));
    hostChallenge->setPlaceholderText("8 bytes");
    hostChallenge->setEnabled(false);

    auto *challenge = new QHBoxLayout;
    challenge->addWidget(challengeRandom);
    challenge->addWidget(challengeFixed);
    challenge->addWidget(hostChallenge, 1);

    auto *layout = new QFormLayout(group);
    layout->setVerticalSpacing(4);
    layout->setContentsMargins(9, 6, 9, 6);
    layout->addRow("Protocol:", protocolCombo);
    layout->addRow("Option \"i\" (hex):", iParameter);
    layout->addRow("Host challenge:", challenge);
    return group;
}


QGroupBox *GpAuthDialog::buildSecurityLevelGroup()
{
    auto *group = new QGroupBox("Security level");

    levelCMac = new QCheckBox("C-MAC (command integrity)");
    levelCMac->setObjectName("levelCMac");
    levelCDec = new QCheckBox("C-DECRYPTION (command confidentiality)");
    levelCDec->setObjectName("levelCDec");
    levelRMac = new QCheckBox("R-MAC (response integrity)");
    levelRMac->setObjectName("levelRMac");
    levelREnc = new QCheckBox("R-ENCRYPTION (response confidentiality)");
    levelREnc->setObjectName("levelREnc");
    levelLabel = new QLabel;
    levelLabel->setObjectName("levelLabel");

    auto *layout = new QVBoxLayout(group);
    layout->addWidget(levelCMac);
    layout->addWidget(levelCDec);
    layout->addWidget(levelRMac);
    layout->addWidget(levelREnc);
    layout->addWidget(levelLabel);
    return group;
}


QGroupBox *GpAuthDialog::buildKeysGroup()
{
    keysGroup = new QGroupBox("Keys");
    keysGroup->setObjectName("keysGroup");

    autodetectKeys = new QCheckBox("Autodetect keys");
    autodetectKeys->setObjectName("autodetectKeys");
    autodetectKeys->setToolTip("Try to find the keys in the known-keys database after the first INITIALIZE UPDATE");

    keyVersion = new QSpinBox;
    keyVersion->setObjectName("keyVersion");
    keyVersion->setRange(0, 0x7F);
    keyVersion->setDisplayIntegerBase(16);
    keyVersion->setPrefix("0x");
    keyVersion->setSpecialValueText("0 (first available)");
    keyVersion->setToolTip("Key Version Number (P1 of INITIALIZE UPDATE); 0 lets the Security Domain choose.");

    keyId = new QSpinBox;
    keyId->setObjectName("keyId");
    keyId->setRange(0, 0x7F);
    keyId->setDisplayIntegerBase(16);
    keyId->setPrefix("0x");
    keyId->setToolTip("Key Identifier of the first key of the set (P2).");

    keySource = new QComboBox;
    keySource->setObjectName("keySource");
    keySource->addItem("Default test keys (40 41 ... 4F)", int(GpAuthParams::KeySource::DefaultTestKeys));
    keySource->addItem("Static keys (ENC, MAC, DEK)", int(GpAuthParams::KeySource::StaticKeys));
    keySource->addItem("Master key with key diversification", int(GpAuthParams::KeySource::MasterKey));

    encKey = hexLineEdit(keysGroup, "encKey");
    macKey = hexLineEdit(keysGroup, "macKey");
    dekKey = hexLineEdit(keysGroup, "dekKey");
    dekKey->setPlaceholderText("optional, needed for PUT KEY");
    masterKey = hexLineEdit(keysGroup, "masterKey");
    for (QLineEdit *edit : {encKey, macKey, dekKey, masterKey})
        edit->setEchoMode(QLineEdit::Password);

    diversification = new QComboBox;
    diversification->setObjectName("diversification");
    diversification->addItem("None", int(GpAuthParams::Diversification::None));
    diversification->addItem("VISA2", int(GpAuthParams::Diversification::Visa2));
    diversification->addItem("EMV CPS 1.1", int(GpAuthParams::Diversification::EmvCps11));
    diversification->setToolTip("Derives the static keys from the master key with the key diversification data "
                                "returned by INITIALIZE UPDATE (issuer specific).");

    showKeys = new QCheckBox("Show key values");
    showKeys->setObjectName("showKeys");

    auto *layout = new QFormLayout(keysGroup);
    layout->addRow(autodetectKeys);
    layout->addRow("Key version:", keyVersion);
    layout->addRow("Key identifier:", keyId);
    layout->addRow("Key source:", keySource);
    layout->addRow("ENC:", encKey);
    layout->addRow("MAC:", macKey);
    layout->addRow("DEK:", dekKey);
    layout->addRow("Master key:", masterKey);
    layout->addRow("Diversification:", diversification);
    layout->addRow(showKeys);
    return keysGroup;
}


QGroupBox *GpAuthDialog::buildScp11Group()
{
    scp11Group = new QGroupBox("Off-card entity (SCP11a / SCP11c)");
    scp11Group->setObjectName("scp11Group");

    oceCertificate = new QLineEdit;
    oceCertificate->setObjectName("oceCertificate");
    oceCertificate->setPlaceholderText("certificate (chain) of the off-card entity");
    oceKey = new QLineEdit;
    oceKey->setObjectName("oceKey");
    oceKey->setPlaceholderText("private key of the off-card entity");

    auto row = [this](QLineEdit *edit, const QString &title) {
        auto *button = new QPushButton("...");
        button->setFixedWidth(32);
        connect(button, &QPushButton::clicked, this, [this, edit, title] { browse(edit, title); });
        auto *box = new QHBoxLayout;
        box->addWidget(edit, 1);
        box->addWidget(button);
        return box;
    };

    auto *layout = new QFormLayout(scp11Group);
    layout->addRow("Certificate:", row(oceCertificate, "Off-card entity certificate"));
    layout->addRow("Private key:", row(oceKey, "Off-card entity private key"));
    return scp11Group;
}


GpAuthParams::Protocol GpAuthDialog::protocol() const
{
    return Protocol(protocolCombo->currentData().toInt());
}


quint8 GpAuthDialog::securityLevel() const
{
    quint8 level = 0;
    if (levelCMac->isChecked()) level |= 0x01;
    if (levelCDec->isChecked()) level |= 0x02;
    if (levelRMac->isChecked()) level |= 0x10;
    if (levelREnc->isChecked()) level |= 0x20;
    return level;
}


void GpAuthDialog::protocolChanged()
{
    const Protocol p = protocol();
    const bool scp11 = p == Protocol::Scp11a || p == Protocol::Scp11b || p == Protocol::Scp11c;

    const QSignalBlocker blocker(iParameter);
    iParameter->clear();
    iParameter->addItem(AutoText);
    iParameter->addItems(iValues(p));
    iParameter->setCurrentIndex(0);
    iParameter->setEnabled(!scp11);

    // levels per protocol: SCP01 has no R-MAC; SCP02 R-ENC does not exist (Table E-11)
    levelRMac->setEnabled(p != Protocol::Scp01);
    levelREnc->setEnabled(p == Protocol::Scp03 || scp11);
    if (!levelRMac->isEnabled()) levelRMac->setChecked(false);
    if (!levelREnc->isEnabled()) levelREnc->setChecked(false);

    challengeRandom->setEnabled(!scp11);
    challengeFixed->setEnabled(!scp11);
    hostChallenge->setEnabled(!scp11 && challengeFixed->isChecked());

    // key identifiers of the SCP11 key pairs: a = 11, b = 13, c = 15 (Amendment F)
    const int defaultId = p == Protocol::Scp11a ? 0x11 : p == Protocol::Scp11b ? 0x13 : p == Protocol::Scp11c ? 0x15 : 0;
    if (keyId->value() == 0 || keyId->value() == 0x11 || keyId->value() == 0x13 || keyId->value() == 0x15)
        keyId->setValue(defaultId);

    scp11Group->setEnabled(p == Protocol::Scp11a || p == Protocol::Scp11c);
    keySource->setEnabled(!scp11);
    keySourceChanged();
}


void GpAuthDialog::keySourceChanged()
{
    const auto source = GpAuthParams::KeySource(keySource->currentData().toInt());
    const bool scp11 = !keySource->isEnabled();
    const bool isStatic = source == GpAuthParams::KeySource::StaticKeys;
    const bool isMaster = source == GpAuthParams::KeySource::MasterKey;

    if (source == GpAuthParams::KeySource::DefaultTestKeys) {
        const QString key = Utility::byteArrayToHexString(
            QByteArray(reinterpret_cast<const char *>(GpAuthParams::DefaultTestKey), sizeof GpAuthParams::DefaultTestKey));
        encKey->setText(key);
        macKey->setText(key);
        dekKey->setText(key);
    }
    for (QLineEdit *edit : {encKey, macKey, dekKey})
        edit->setEnabled(isStatic && !scp11);
    masterKey->setEnabled(isMaster && !scp11);
    diversification->setEnabled(isMaster && !scp11);
    refresh();
}


void GpAuthDialog::browse(QLineEdit *target, const QString &title)
{
    const QString file = QFileDialog::getOpenFileName(
        this, title, QString(), "Certificates and keys (*.pem *.der *.crt *.cer *.key *.p12 *.pfx);;All files (*)");
    if (!file.isEmpty())
        target->setText(file);
}


QString GpAuthDialog::problem() const
{
    const Protocol p = protocol();

    if (selectFirst->isChecked()) {
        const QString digits = cleanHex(targetAid->currentText());
        if (digits.size() % 2 != 0)
            return "The AID has an odd number of hex digits.";
        if (digits.size() < 10 || digits.size() > 32)
            return "The AID must be 5 to 16 bytes long.";
    }

    const QString i = iParameter->currentText().trimmed();
    if (iParameter->isEnabled() && i != AutoText && i.size() != 2)
        return "The option \"i\" must be \"Auto\" or two hex digits.";

    if (challengeFixed->isChecked() && challengeFixed->isEnabled() && cleanHex(hostChallenge->text()).size() != 16)
        return "The host challenge must be 8 bytes long.";

    if (p == Protocol::Scp11a || p == Protocol::Scp11c) {
        if (!QFileInfo::exists(oceCertificate->text()))
            return "Select the certificate of the off-card entity.";
        if (!QFileInfo::exists(oceKey->text()))
            return "Select the private key of the off-card entity.";
        return QString();
    }
    if (p == Protocol::Scp11b)
        return QString();

    const QList<int> lengths = p == Protocol::Scp03 ? QList<int>{16, 24, 32} : QList<int>{16};
    const QString allowed = p == Protocol::Scp03 ? "16, 24 or 32 bytes (AES)" : "16 bytes (3DES)";
    auto check = [&](const QString &name, const QString &text, bool required) -> QString {
        const QString digits = cleanHex(text);
        if (digits.isEmpty())
            return required ? QString("The %1 key is missing.").arg(name) : QString();
        if (digits.size() % 2 != 0)
            return QString("The %1 key has an odd number of hex digits.").arg(name);
        if (!lengths.contains(digits.size() / 2))
            return QString("The %1 key must be %2 long.").arg(name, allowed);
        return QString();
    };

    const auto source = GpAuthParams::KeySource(keySource->currentData().toInt());
    if (source == GpAuthParams::KeySource::StaticKeys) {
        for (const QString &error : {check("ENC", encKey->text(), true), check("MAC", macKey->text(), true),
                                     check("DEK", dekKey->text(), false)}) {
            if (!error.isEmpty())
                return error;
        }
        if (cleanHex(encKey->text()).size() != cleanHex(macKey->text()).size())
            return "The ENC and MAC keys must have the same length.";
    } else if (source == GpAuthParams::KeySource::MasterKey) {
        return check("master", masterKey->text(), true);
    }
    return QString();
}


void GpAuthDialog::refresh()
{
    const QString level = GpCoding::securityLevelText(securityLevel());
    levelLabel->setText(QString("P1 = %1: %2").arg(hex2(securityLevel()), level));

    const QString error = problem();
    problemLabel->setText(error);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(error.isEmpty());
}


void GpAuthDialog::setParams(const GpAuthParams &params)
{
    protocolCombo->setCurrentIndex(protocolCombo->findData(int(params.protocol)));

    selectFirst->setChecked(params.selectFirst);
    targetAid->setCurrentText(Utility::byteArrayToHexString(params.targetAid));
    targetAid->setEnabled(params.selectFirst);

    levelCMac->setChecked(params.securityLevel & 0x01);
    levelCDec->setChecked(params.securityLevel & 0x02);
    levelRMac->setChecked(params.securityLevel & 0x10);
    levelREnc->setChecked(params.securityLevel & 0x20);

    keyVersion->setValue(params.keyVersion);
    keyId->setValue(params.keyId);
    keySource->setCurrentIndex(keySource->findData(int(params.keySource)));
    if (params.keySource == GpAuthParams::KeySource::StaticKeys) {
        encKey->setText(Utility::byteArrayToHexString(params.encKey));
        macKey->setText(Utility::byteArrayToHexString(params.macKey));
        dekKey->setText(Utility::byteArrayToHexString(params.dekKey));
    }
    masterKey->setText(Utility::byteArrayToHexString(params.masterKey));
    diversification->setCurrentIndex(diversification->findData(int(params.diversification)));

    challengeFixed->setChecked(!params.hostChallenge.isEmpty());
    challengeRandom->setChecked(params.hostChallenge.isEmpty());
    hostChallenge->setText(Utility::byteArrayToHexString(params.hostChallenge));

    oceCertificate->setText(params.oceCertificateFile);
    oceKey->setText(params.oceKeyFile);
    readRegistry->setChecked(params.readRegistry);

    protocolChanged();
    if (params.iParameter >= 0)
        iParameter->setCurrentText(hex2(params.iParameter));
    refresh();
}


GpAuthParams GpAuthDialog::params() const
{
    GpAuthParams r;
    r.protocol = protocol();
    const QString i = iParameter->currentText().trimmed();
    r.iParameter = (!iParameter->isEnabled() || i == AutoText || i.isEmpty()) ? -1 : i.toInt(nullptr, 16);
    r.securityLevel = securityLevel();

    r.selectFirst = selectFirst->isChecked();
    r.targetAid = Utility::hexStringToByteArray(targetAid->currentText());

    r.keyVersion = quint8(keyVersion->value());
    r.keyId = quint8(keyId->value());
    r.keySource = GpAuthParams::KeySource(keySource->currentData().toInt());
    r.encKey = Utility::hexStringToByteArray(encKey->text());
    r.macKey = Utility::hexStringToByteArray(macKey->text());
    r.dekKey = Utility::hexStringToByteArray(dekKey->text());
    if (r.keySource == GpAuthParams::KeySource::MasterKey) {
        r.masterKey = Utility::hexStringToByteArray(masterKey->text());
        r.encKey.clear();
        r.macKey.clear();
        r.dekKey.clear();
    }
    r.diversification = GpAuthParams::Diversification(diversification->currentData().toInt());

    if (challengeFixed->isChecked() && challengeFixed->isEnabled())
        r.hostChallenge = Utility::hexStringToByteArray(hostChallenge->text());

    r.oceCertificateFile = oceCertificate->text();
    r.oceKeyFile = oceKey->text();
    r.readRegistry = readRegistry->isChecked();
    return r;
}
