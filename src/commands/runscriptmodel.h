#ifndef RUNSCRIPT_MODEL_H
#define RUNSCRIPT_MODEL_H

#include <QAbstractListModel>
#include <QBrush>
#include <QColor>
#include <QHash>
#include <QList>

#include "command.h"


class RunScriptModel : public QAbstractListModel {
    Q_OBJECT

public:
    // result of the last run, shown as row colour
    enum class Status { None, Running, Passed, Failed };

    RunScriptModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    void addCommand(const QSharedPointer<Command> &command) {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        m_commands.append(command);
        endInsertRows();
    }

    void insertCommands(int row, const QList<QSharedPointer<Command>> &commands) {
        if (row < 0 || row > m_commands.size())
            return;

        beginInsertRows(QModelIndex(), row, row + commands.size() - 1);

        int pos = row;
        for (const auto &cmd : commands)
            m_commands.insert(pos++, cmd);

        endInsertRows();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return m_commands.count();
    }

    QSharedPointer<Command> itemFromIndex(const QModelIndex &index) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_commands.count())
            return nullptr;

        return m_commands[index.row()];
    }

    QSharedPointer<Command> item(int row) {
        if (row < 0 || row >= rowCount())
            return nullptr;

        return itemFromIndex(index(row, 0));
    }

    QList<QSharedPointer<Command>> takeCommands(int row) {
        if (row < 0 || row >= m_commands.size())
            return {};

        beginRemoveRows(QModelIndex(), row, row);

        QList<QSharedPointer<Command>> removedCommands;
        removedCommands.append(m_commands.takeAt(row));

        endRemoveRows();

        return removedCommands;
    }

    Status status(int row) const {
        if (row < 0 || row >= m_commands.size())
            return Status::None;
        return m_status.value(m_commands[row].data(), Status::None);
    }

    void setStatus(int row, Status status) {
        if (row < 0 || row >= m_commands.size())
            return;
        if (status == Status::None)
            m_status.remove(m_commands[row].data());
        else
            m_status.insert(m_commands[row].data(), status);
        emit dataChanged(index(row, 0), index(row, 0), {Qt::BackgroundRole});
    }

    void clearStatus() {
        if (m_status.isEmpty())
            return;
        m_status.clear();
        if (!m_commands.isEmpty())
            emit dataChanged(index(0, 0), index(m_commands.size() - 1, 0), {Qt::BackgroundRole});
    }

    bool removeCommand(int row) {
        if (row < 0 || row >= m_commands.size())
            return false;

        m_status.remove(m_commands[row].data());
        beginRemoveRows(QModelIndex(), row, row);
        m_commands.removeAt(row);
        endRemoveRows();

        return true;
    }

    void clear() {
        beginResetModel();
        m_commands.clear();
        m_status.clear();
        endResetModel();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_commands.count())
            return QVariant();

        const QSharedPointer<Command> &command = m_commands[index.row()];

        if (role == Qt::DisplayRole)
            return command->commandName;
        else if (role == Qt::UserRole)
            return QVariant::fromValue(command);
        else if (role == Qt::BackgroundRole) {
            switch (m_status.value(command.data(), Status::None)) {
            case Status::Running: return QBrush(QColor(255, 240, 160));
            case Status::Passed: return QBrush(QColor(200, 240, 200));
            case Status::Failed: return QBrush(QColor(250, 190, 190));
            default: break;
            }
        }

        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override {
        if (role == Qt::UserRole) {
            if (index.isValid() && index.row() >= 0 && index.row() < m_commands.count() && value.canConvert<QSharedPointer<Command>>()) {
                m_status.remove(m_commands[index.row()].data());
                m_commands[index.row()] = value.value<QSharedPointer<Command>>();
                emit dataChanged(index, index);
                return true;
            }
        }
        return false;
    }

    // moves the (sorted) rows so that they end up in front of row 'target' (== rowCount() appends)
    void moveToRow(const QList<int> &rows, int target) {
        QList<int> order;      // old row of every new position
        for (int r = 0; r < target && r < m_commands.size(); ++r)
            if (!rows.contains(r))
                order.append(r);
        order.append(rows);
        for (int r = target; r < m_commands.size(); ++r)
            if (!rows.contains(r))
                order.append(r);
        if (order.size() != m_commands.size())
            return;

        emit layoutAboutToBeChanged();
        const QModelIndexList old = persistentIndexList();
        QList<QSharedPointer<Command>> moved;
        for (int r : order)
            moved.append(m_commands.at(r));
        m_commands = moved;
        QModelIndexList updated;
        for (const QModelIndex &index : old)
            updated.append(createIndex(order.indexOf(index.row()), 0));
        changePersistentIndexList(old, updated);
        emit layoutChanged();
    }

    QString commandType(int row) const {
        if (row >= 0 && row < m_commands.count())
            return m_commands[row]->typeName();

        return QString();
    }

    QList<QSharedPointer<Command>> commands() const {
        return m_commands;
    }

private:
    QList<QSharedPointer<Command>> m_commands;
    QHash<const Command *, Status> m_status;
};

#endif
