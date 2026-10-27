#ifndef SCRIPTRUNNER_H
#define SCRIPTRUNNER_H

#include <QObject>
#include <QList>
#include <QSharedPointer>

#include <atomic>

#include "command.h"
#include "global.h"

class CardSession;
class QThread;


// Executes a list of commands one after the other on a worker thread; by default it stops at the
// first command that fails, otherwise it runs on and reports the failures. The commands are copied at start(), later edits do not affect a run.
// All signals are delivered on the thread of the runner.
class ScriptRunner : public QObject
{
    Q_OBJECT

public:
    explicit ScriptRunner(CardSession *session, QObject *parent = nullptr);
    ~ScriptRunner() override;

    bool isRunning() const { return m_running; }

    // firstIndex is the position of commands.first() in the caller's list and is only used to
    // number the signals; returns false if a run is already active or there is nothing to run
    bool start(const QList<QSharedPointer<Command>> &commands, int firstIndex = 0, bool stopOnError = true);
    // the current command is finished (waiting commands are interrupted), the rest is skipped
    // rows: the position of each command in the caller's list (same size as commands)
    bool start(const QList<QSharedPointer<Command>> &commands, const QList<int> &rows, bool stopOnError = true);
    void stop();

signals:
    void commandStarted(int index);
    void commandFinished(int index, bool ok);
    // success: no command failed; executed: number of commands that were run
    void finished(bool success, int executed);
    void logBytes(LogType type, const QByteArray &bytes);
    void logString(LogType type, const QString &text);

private:
    void run(const QList<QSharedPointer<Command>> &commands, const QList<int> &rows, bool stopOnError);
    void threadDone(bool success, int executed);

    CardSession *m_session;
    QThread *m_thread = nullptr;
    std::atomic<bool> m_cancel{false};
    bool m_running = false;     // GUI thread only
};

#endif
