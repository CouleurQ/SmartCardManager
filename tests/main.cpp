// Test runner. Usage: tst_smartcardmanager [ClassName ...] [-- QtTest options]
// A ScmApplication is required because the UI classes downcast qApp to it.
// Run headless with: QT_QPA_PLATFORM=offscreen (make.sh test does this).
#include "scmapplication.h"
#include "testregistry.h"

#include <QTest>
#include <QStringList>

int main(int argc, char *argv[])
{
    ScmApplication app(argc, argv);
    app.setApplicationName("SmartCardManagerTest");

    // Split "ClassName..." filters from QtTest arguments.
    QStringList filters;
    QStringList qtestArgs{argv[0]};
    bool afterSeparator = false;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--")
            afterSeparator = true;
        else if (afterSeparator || arg.startsWith('-'))
            qtestArgs << arg;
        else
            filters << arg;
    }

    int failures = 0;
    for (const auto &factory : TestRegistry::factories()) {
        QObject *test = factory();
        const QString name = test->metaObject()->className();
        if (filters.isEmpty() || filters.contains(name))
            failures += QTest::qExec(test, qtestArgs);
        delete test;
    }
    return failures;
}
