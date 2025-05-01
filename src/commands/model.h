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

    void insertRow(int row, const QList<QSharedPointer<Command>> &commands) {
        if (row < 0 || row > m_commands.size())
            return;

        beginInsertRows(QModelIndex(), row, row + commands.size() - 1);

        for (const auto &cmd : commands)
            m_commands.insert(row, cmd);

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

    QList<QSharedPointer<Command>> takeRow(int row) {
        if (row < 0 || row >= m_commands.size())
            return {};

        beginRemoveRows(QModelIndex(), row, row);

        QList<QSharedPointer<Command>> removedCommands;
        removedCommands.append(m_commands.takeAt(row));

        endRemoveRows();

        return removedCommands;
    }

    bool removeRow(int row, const QModelIndex &parent = QModelIndex()) {
        if (row < 0 || row >= m_commands.size())
            return false;

        beginRemoveRows(parent, row, row);
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
        if (!index.isValid())
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
            if (index.isValid() && value.canConvert<QSharedPointer<Command>>()) {
                m_commands[index.row()] = value.value<QSharedPointer<Command>>();
                emit dataChanged(index, index);
                return true;
            }
        }
        return false;
    }

    Command::ObjectType commandType(int row) const {
        if (row >= 0 && row < m_commands.count())
            return m_commands[row]->type();

        return Command::ObjectType::BaseType;
    }

private:
    QList<QSharedPointer<Command>> m_commands;
};

#endif
