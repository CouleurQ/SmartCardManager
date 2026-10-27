#ifndef GPAUTHDIALOG_H
#define GPAUTHDIALOG_H

#include <QDialog>

#include "card/gpauthparams.h"

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QRadioButton;
class QSpinBox;


// Collects the parameters for opening a GlobalPlatform secure channel to a Security Domain.
class GpAuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GpAuthDialog(QWidget *parent = nullptr);

    void setParams(const GpAuthParams &params);
    GpAuthParams params() const;

private:
    QGroupBox *buildDomainGroup();
    QGroupBox *buildProtocolGroup();
    QGroupBox *buildSecurityLevelGroup();
    QGroupBox *buildKeysGroup();
    QGroupBox *buildScp11Group();

    GpAuthParams::Protocol protocol() const;
    quint8 securityLevel() const;
    void protocolChanged();
    void keySourceChanged();
    void securityLevelEdited();
    void browse(QLineEdit *target, const QString &title);
    void refresh();
    QString problem() const;

    QGroupBox *domainGroup;
    QCheckBox *selectFirst;
    QComboBox *targetAid;

    QComboBox *protocolCombo;
    QComboBox *iParameter;
    QRadioButton *challengeRandom;
    QRadioButton *challengeFixed;
    QLineEdit *hostChallenge;

    QCheckBox *levelCMac;
    QCheckBox *levelCDec;
    QCheckBox *levelRMac;
    QCheckBox *levelREnc;
    QLabel *levelLabel;

    QGroupBox *keysGroup;
    QCheckBox *autodetectKeys;
    QSpinBox *keyVersion;
    QSpinBox *keyId;
    QComboBox *keySource;
    QLineEdit *encKey;
    QLineEdit *macKey;
    QLineEdit *dekKey;
    QLineEdit *masterKey;
    QComboBox *diversification;
    QCheckBox *showKeys;

    QGroupBox *scp11Group;
    QLineEdit *oceCertificate;
    QLineEdit *oceKey;

    QCheckBox *readRegistry;
    QLabel *problemLabel;
    QDialogButtonBox *buttons;
};

#endif
