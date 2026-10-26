#ifndef RUNSCRIPT_MODEL_H
#define RUNSCRIPT_MODEL_H

#include <QAbstractListModel>
#include <QList>

#include "command.h"


class RunScriptModel : public QAbstractListModel {
    Q_OBJECT

public:
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

    bool removeCommand(int row) {
        if (row < 0 || row >= m_commands.size())
            return false;

        beginRemoveRows(QModelIndex(), row, row);
        m_commands.removeAt(row);
        endRemoveRows();

        return true;
    }

    void clear() {
        beginResetModel();
        m_commands.clear();
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

        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override {
        if (role == Qt::UserRole) {
            if (index.isValid() && index.row() >= 0 && index.row() < m_commands.count() && value.canConvert<QSharedPointer<Command>>()) {
                m_commands[index.row()] = value.value<QSharedPointer<Command>>();
                emit dataChanged(index, index);
                return true;
            }
        }
        return false;
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
};

#endif
