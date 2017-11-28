/*
 * DesktopUtils.cpp
 *
 * Copyright (C) 2009-17 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include "DesktopUtils.hpp"

#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QEventLoop>
#include <QDesktopServices>

#include <boost/atomic.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/ref.hpp>

#include <core/Error.hpp>
#include <core/FilePath.hpp>
#include <core/FileSerializer.hpp>
#include <core/system/Process.hpp>
#include <core/system/System.hpp>
#include <core/system/Environment.hpp>

#include "DesktopOptions.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

using namespace rstudio::core;

namespace rstudio {
namespace desktop {

#ifdef Q_OS_WIN

void reattachConsoleIfNecessary()
{
   if (::AttachConsole(ATTACH_PARENT_PROCESS))
   {
      freopen("CONOUT$","wb",stdout);
      freopen("CONOUT$","wb",stderr);
      freopen("CONIN$","rb",stdin);
      std::ios::sync_with_stdio();
   }
}

#else

void reattachConsoleIfNecessary()
{

}

#endif

// NOTE: this code is duplicated in diagnostics as well (and also in
// SessionOptions.hpp although the code path isn't exactly the same)
FilePath userLogPath()
{
   FilePath userHomePath = core::system::userHomePath("R_USER|HOME");
   FilePath logPath = core::system::userSettingsPath(
         userHomePath,
         "RStudio-Desktop").childPath("log");
   return logPath;
}

#ifndef Q_OS_MAC
double devicePixelRatio(QMainWindow* pMainWindow)
{
   return 1.0;
}

bool isOSXMavericks()
{
   return false;
}

// NOTE: also RHEL
bool isCentOS()
{
   FilePath redhatRelease("/etc/redhat-release");
   if (!redhatRelease.exists())
      return false;

   std::string contents;
   Error error = readStringFromFile(redhatRelease, &contents);
   if (error)
      return false;

   return contents.find("CentOS") != std::string::npos ||
          contents.find("Red Hat Enterprise Linux") != std::string::npos;
}

void enableFullscreenMode(QMainWindow* pMainWindow, bool primary)
{

}

void toggleFullscreenMode(QMainWindow* pMainWindow)
{

}

bool supportsFullscreenMode(QMainWindow* pMainWindow)
{
   return false;
}

void initializeLang()
{
}

#endif

void raiseAndActivateWindow(QWidget* pWindow)
{
   // WId wid = pWindow->effectiveWinId(); -- gets X11 window id
   // gtk_window_present_with_time(GTK_WINDOW, timestamp)

   if (pWindow->isMinimized())
   {
      pWindow->setWindowState(
                     pWindow->windowState() & ~Qt::WindowMinimized);
   }

   pWindow->raise();
   pWindow->activateWindow();
}

void moveWindowBeneath(QWidget* pTop, QWidget* pBottom)
{
#ifdef WIN32
   HWND hwndTop = reinterpret_cast<HWND>(pTop->winId());
   HWND hwndBottom = reinterpret_cast<HWND>(pBottom->winId());
   ::SetWindowPos(hwndBottom, hwndTop, 0, 0, 0, 0,
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
   // not currently supported on Linux--Qt doesn't provide a way to view or
   // change the window stacking order
}

void closeWindow(QWidget* pWindow)
{
   pWindow->close();
}

QMessageBox::Icon safeMessageBoxIcon(QMessageBox::Icon icon)
{
   // if a gtk theme has a missing or corrupt icon for one of the stock
   // dialog images, qt crashes when attempting to show the dialog
#ifdef Q_OS_LINUX
   return QMessageBox::NoIcon;
#else
   return icon;
#endif
}


bool showYesNoDialog(QMessageBox::Icon icon,
                     QWidget *parent,
                     const QString &title,
                     const QString& text)
{
   // basic message box attributes
   QMessageBox messageBox(safeMessageBoxIcon(icon),
                          title,
                          text,
                          QMessageBox::NoButton,
                          parent);
   messageBox.setWindowModality(Qt::WindowModal);

   // initialize buttons
   QPushButton* pYes = new QPushButton(QString::fromUtf8("Yes"));
   messageBox.addButton(pYes, QMessageBox::YesRole);
   messageBox.addButton(new QPushButton(QString::fromUtf8("No")), QMessageBox::NoRole);
   messageBox.setDefaultButton(pYes);

   // show the dialog modally
   messageBox.exec();

   // return true if the user clicked yes
   return messageBox.clickedButton() == pYes;
}

void showMessageBox(QMessageBox::Icon icon,
                    QWidget *parent,
                    const QString &title,
                    const QString& text)
{
   // basic message box attributes
   QMessageBox messageBox(safeMessageBoxIcon(icon),
                          title,
                          text,
                          QMessageBox::NoButton,
                          parent);
   messageBox.setWindowModality(Qt::WindowModal);
   messageBox.addButton(new QPushButton(QString::fromUtf8("OK")), QMessageBox::AcceptRole);
   messageBox.exec();
}

void showWarning(QWidget *parent, const QString &title, const QString& text)
{
   showMessageBox(QMessageBox::Warning, parent, title, text);
}

void showInfo(QWidget* parent, const QString& title, const QString& text)
{
   showMessageBox(QMessageBox::Information, parent, title, text);
}

void showFileError(const QString& action,
                   const QString& file,
                   const QString& error)
{
   QString msg = QString::fromUtf8("Error ") + action +
                 QString::fromUtf8(" ") + file +
                 QString::fromUtf8(" - ") + error;
   showMessageBox(QMessageBox::Critical,
                  nullptr,
                  QString::fromUtf8("File Error"),
                  msg);
}

bool isFixedWidthFont(const QFont& font)
{
   QFontMetrics metrics(font);
   int width = metrics.width(QChar::fromLatin1(' '));
   char chars[] = {'m', 'i', 'A', '/', '-', '1', 'l', '!', 'x', 'X', 'y', 'Y'};
   for (char i : chars)
   {
      if (metrics.width(QChar::fromLatin1(i)) != width)
         return false;
   }
   return true;
}

int getDpi()
{
#ifdef _WIN32
   HDC defaultDC = GetDC(NULL);
   int dpi = GetDeviceCaps(defaultDC, LOGPIXELSX);
   ReleaseDC(NULL, defaultDC);
   return dpi;
#else
   // presume 96 DPI on other Qt platforms (i.e. Linux) for now
   return 96;
#endif
}

double getDpiZoomScaling()
{
   double dpiZoomScaling = 1.0;
   int dpi = getDpi();
   if (dpi >= 192)
   {
      // Corresponds to 200% scaling (introduced in Windows 8.1)
      dpiZoomScaling = 1.5;
   }
   else if (dpi >= 144)
   {
      // Corresponds to 150% scaling
      dpiZoomScaling = 1.2;
   }
   return dpiZoomScaling;
}

#ifdef _WIN32

// on Win32 open urls using our special urlopener.exe -- this is
// so that the shell exec is made out from under our windows "job"
void openUrl(const QUrl& url)
{
   // we allow default handling for  mailto and file schemes because qt
   // does custom handling for them and they aren't affected by the chrome
   //job object issue noted above
   if (url.scheme() == QString::fromUtf8("mailto") ||
       url.scheme() == QString::fromUtf8("file"))
   {
      QDesktopServices::openUrl(url);
   }
   else
   {
      core::system::ProcessOptions options;
      options.breakawayFromJob = true;
      options.detachProcess = true;

      std::vector<std::string> args;
      args.push_back(url.toString().toStdString());

      core::system::ProcessResult result;
      Error error = core::system::runProgram(
            desktop::options().urlopenerPath().absolutePath(),
            args,
            "",
            options,
            &result);

      if (error)
         LOG_ERROR(error);
      else if (result.exitStatus != EXIT_SUCCESS)
         LOG_ERROR_MESSAGE(result.stdErr);
   }
}

// Qt 4.8.3 on Win7 (32-bit) has problems with opening the ~ directory
// (it attempts to navigate to the "Documents library" and then hangs)
// So we use the Qt file dialog implementations when we are running
// on Win32
QFileDialog::Options standardFileDialogOptions()
{
    bool isWindowsXP = QSysInfo::windowsVersion() == QSysInfo::WV_XP;
    if (isWindowsXP || core::system::isWin64())
        return 0;
    else
        return QFileDialog::DontUseNativeDialog;
}

#else

void openUrl(const QUrl& url)
{
   QDesktopServices::openUrl(url);
}

QFileDialog::Options standardFileDialogOptions()
{
   return nullptr;
}

#endif

} // namespace desktop
} // namespace rstudio
