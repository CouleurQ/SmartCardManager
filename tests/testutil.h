#ifndef TESTUTIL_H
#define TESTUTIL_H

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QPointer>

// Drives the modal dialogs opened by the static QFileDialog/QMessageBox helpers: while the test
// runs a blocking call (e.g. the Save button), a timer fills in the file name and confirms every
// dialog that appears. Requires a non-native dialog (true for the offscreen platform).
class DialogAutomator : public QObject
{
public:
    explicit DialogAutomator(const QString &filePath = QString())
        : m_path(filePath)
    {
        connect(&m_timer, &QTimer::timeout, this, &DialogAutomator::poll);
        m_timer.start(20);
    }

    int fileDialogsHandled = 0;
    int messageBoxesHandled = 0;

private:
    void poll()
    {
        QWidget *modal = QApplication::activeModalWidget();
        if (!modal) {
            // The offscreen platform may not report an active window; look for visible dialogs.
            for (QWidget *w : QApplication::topLevelWidgets()) {
                if (w->isVisible() && (qobject_cast<QFileDialog *>(w) || qobject_cast<QMessageBox *>(w))) {
                    modal = w;
                    break;
                }
            }
        }
        if (!modal)
            return;
        if (auto *fd = qobject_cast<QFileDialog *>(modal)) {
            if (!m_path.isEmpty())
                fd->selectFile(m_path);
            ++fileDialogsHandled;
            static_cast<QDialog *>(fd)->accept();   // QFileDialog::accept() is protected
        } else if (auto *mb = qobject_cast<QMessageBox *>(modal)) {
            ++messageBoxesHandled;
            mb->accept();
        }
    }

    QString m_path;
    QTimer m_timer;
};

#endif
