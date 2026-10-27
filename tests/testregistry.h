#ifndef TESTREGISTRY_H
#define TESTREGISTRY_H

#include <QObject>
#include <QList>
#include <functional>

// Lets every tst_*.cpp register its QObject test class with the single test executable.
struct TestRegistry {
    using Factory = std::function<QObject *()>;

    static QList<Factory> &factories() {
        static QList<Factory> list;
        return list;
    }
};

template <typename T>
struct TestRegistrar {
    TestRegistrar() { TestRegistry::factories().append([]() -> QObject * { return new T; }); }
};

#endif
