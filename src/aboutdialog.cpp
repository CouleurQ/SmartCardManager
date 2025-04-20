#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QLabel>


AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    setApplicationIcon(windowIcon().pixmap(48, 48));
    setApplicationName(QApplication::applicationName());
    setApplicationVersion(QApplication::applicationVersion());
    setCopyrightInfo(tr("Copyright © 2025 %1").arg(QApplication::organizationName()));
    setDescription(tr("<p>Platform independent application to handle smartcard communication, Java Card managemnt for Linux/Windows/MacOS.</p>"
                                   "<p>Credits: anyone</p>"));

}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::setApplicationIcon(const QPixmap &icon)
{
    ui->iconLabel->setPixmap(icon.scaled(48, 48, Qt::KeepAspectRatio));
}

void AboutDialog::setApplicationName(const QString &name)
{
    ui->titleLabel->setText(name);
}

void AboutDialog::setApplicationVersion(const QString &version)
{
    ui->versionLabel->setText(tr("Version %1").arg(version));
}

void AboutDialog::setCopyrightInfo(const QString &info)
{
    ui->copyrightLabel->setText(info);
}

void AboutDialog::setDescription(const QString &description)
{
    ui->descriptionBrowser->setHtml(description); // Or ui->descriptionBrowser->setPlainText(description);
}
