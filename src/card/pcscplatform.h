#ifndef PCSCPLATFORM_H
#define PCSCPLATFORM_H

// Platform specific PC/SC headers (the API is the same: SCardEstablishContext, ...)

#include <QtGlobal>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <winscard.h>
#elif defined(Q_OS_MACOS)
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <pcsclite.h>
#include <winscard.h>
#endif

// not defined by the Windows headers
#ifndef MAX_ATR_SIZE
#define MAX_ATR_SIZE 33
#endif

#endif
