#ifndef PRIVATEACCESS_H
#define PRIVATEACCESS_H

// The tab handlers expose their behaviour only through private methods (slots wired to buttons
// and context menus). Until they get a public/testable interface (see docs/CODE_REVIEW.md, S-07)
// the UI tests reach them by lifting `private` for these headers only.
// All Qt/system headers are pulled in first so only the project headers are affected.
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtNetwork>
#include <QtSvgWidgets>
#include "scmapplication.h"
#include "hexedit.h"
#include "global.h"
#include "commands/model.h"
#include "commands/view.h"

#define private public
#include "ui/tabhandlersendcommand.h"
#include "ui/tabhandlerrunscript.h"
#include "ui/mainwindow.h"
#undef private

#endif
