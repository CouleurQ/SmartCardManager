#include "scriptrunner.h"

#include "card/cardsession.h"
#include "utility.h"

#include <QMetaObject>
#include <QThread>


ScriptRunner::ScriptRunner(CardSession *session, QObject *parent)
    : QObject(parent)
    , m_session(session)
{
    Q_ASSERT(session != nullptr);
}


ScriptRunner::~ScriptRunner()
{
    if (m_thread) {
        m_cancel = true;
        m_thread->wait();
        delete m_thread;
    }
}


bool ScriptRunner::start(const QList<QSharedPointer<Command>> &commands, int firstIndex, bool stopOnError)
{
    QList<int> rows;
    for (int i = 0; i < commands.size(); ++i)
        rows.append(firstIndex + i);
    return start(commands, rows, stopOnError);
}


bool ScriptRunner::start(const QList<QSharedPointer<Command>> &commands, const QList<int> &rows, bool stopOnError)
{
    if (m_running || commands.isEmpty() || rows.size() != commands.size())
        return false;

    QList<QSharedPointer<Command>> copies;
    for (const auto &command : commands)
        copies.append(command->clone());

    m_cancel = false;
    m_running = true;
    m_thread = QThread::create([this, copies, rows, stopOnError]() { run(copies, rows, stopOnError); });
    m_thread->start();
    return true;
}


void ScriptRunner::stop()
{
    m_cancel = true;
}


void ScriptRunner::run(const QList<QSharedPointer<Command>> &commands, const QList<int> &rows, bool stopOnError)
{
    bool success = true;
    int executed = 0;
    int failed = 0;

    for (int i = 0; i < commands.size(); ++i) {
        if (m_cancel) {
            emit logString(LogType::Warning, "Script stopped");
            success = false;
            break;
        }

        const int index = rows.at(i);
        const Command &command = *commands.at(i);
        emit commandStarted(index);
        emit logString(LogType::Terminal, QString("Script step %1/%2: %3").arg(i + 1).arg(commands.size()).arg(command.commandName));

        const QByteArray request = command.requestBytes();
        if (!request.isEmpty())
            emit logBytes(LogType::CardCmd, request);

        QByteArray response;
        const bool ok = command.execute(*m_session, &response, &m_cancel);
        ++executed;

        if (!response.isEmpty())
            emit logBytes(LogType::CardRsp, response);

        if (!ok) {
            QString reason;
            if (m_cancel)
                reason = "cancelled";
            else if (request.isEmpty())
                reason = "terminal command failed";
            else if (response.isEmpty())
                reason = "no response from the card";
            else
                reason = QString("unexpected response, expected %1").arg(Utility::byteArrayToHexString(command.expectedResponse));
            emit logString(LogType::Error, QString("Script step %1 failed: %2").arg(i + 1).arg(reason));
        }

        emit commandFinished(index, ok);
        if (!ok) {
            success = false;
            ++failed;
            if (stopOnError || m_cancel)
                break;
        }
    }

    if (success)
        emit logString(LogType::Info, QString("Script finished: %1 command(s) passed").arg(executed));
    else if (failed > 0 && !m_cancel && !stopOnError)
        emit logString(LogType::Error, QString("Script finished: %1 of %2 command(s) failed").arg(failed).arg(executed));

    QMetaObject::invokeMethod(this, [this, success, executed]() { threadDone(success, executed); }, Qt::QueuedConnection);
}


void ScriptRunner::threadDone(bool success, int executed)
{
    if (m_thread) {
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;
    }
    m_running = false;
    emit finished(success, executed);
}
