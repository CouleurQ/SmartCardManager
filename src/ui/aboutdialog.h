#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>


namespace Ui {
class AboutDialog;
}


class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

    void setApplicationIcon(const QPixmap &icon);
    void setApplicationName(const QString &name);
    void setApplicationVersion(const QString &version);
    void setCopyrightInfo(const QString &info);
    void setDescription(const QString &description);

private:
    Ui::AboutDialog *ui;
};

#endif
