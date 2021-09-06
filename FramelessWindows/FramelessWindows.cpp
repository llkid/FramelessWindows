#include "FramelessWindows.h"

#include <qapplication.h>
#include <qdebug.h>
#include <qmessagebox.h>
#include <qoperatingsystemversion.h>
#include <qscreen.h>

#ifdef Q_OS_WIN

#include <dwmapi.h>
#include <windowsx.h>

#pragma comment(lib, "Dwmapi.lib")

#endif  // Q_OS_WIN

#include <QCloseEvent>
#include <QMenu>

#include "DPIMonitor.h"

FramelessWindows::FramelessWindows(QWidget* parent)
    : QWidget(parent), borderSize(4), max_min_count(0) {
  ui.setupUi(this);

  setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);

#ifdef Q_OS_WIN

  HWND hwnd = reinterpret_cast<HWND>(this->winId());
  DWORD style = GetWindowLong(hwnd, GWL_STYLE);
  SetWindowLongPtr(hwnd, GWL_STYLE,
                   style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);

  BOOL pfEnabled;
  DwmIsCompositionEnabled(&pfEnabled);
  if (pfEnabled) {
    SetWindowLongPtr(
        hwnd, GWL_STYLE,
        style | WS_THICKFRAME | WS_CAPTION | WS_BORDER | WS_MAXIMIZEBOX);
    MARGINS margins{1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
  }

#endif  // Q_OS_WIN

  connect(ui.bt_close, &QPushButton::clicked, this, &FramelessWindows::close);
  connect(ui.bt_maxmize, &QPushButton::clicked, [this] {
    /*auto hwnd = reinterpret_cast<HWND>(winId());
    ShowWindow(hwnd, IsZoomed(hwnd) ? SW_SHOWNORMAL : SW_SHOWMAXIMIZED);*/
    if (!this->isMaximized()) {
      this->showMaximized();
    } else {
      this->showNormal();
    }
  });
  connect(ui.bt_minmize, &QPushButton::clicked, [this] {
    // ShowWindow(reinterpret_cast<HWND>(winId()), SW_SHOWMINIMIZED);
    this->showMinimized();
  });

  auto current = QOperatingSystemVersion::current();
  qDebug() << "SystemVersion" << current;
  qDebug() << "majorVersion" << current.majorVersion();
  qDebug() << "minorVersion" << current.minorVersion();
  qDebug() << "microVersion" << current.microVersion();
  qDebug() << "name" << current.name();
  qDebug() << "segmentCount" << current.segmentCount();

  bool on = current.isAnyOfType(
      {QOperatingSystemVersion::Windows, QOperatingSystemVersion::MacOS,
       QOperatingSystemVersion::IOS, QOperatingSystemVersion::Android});
  qDebug() << (on ? "os type is right" : "os type is incorrect");

  trayIcon.reset(new QSystemTrayIcon(
      QApplication::style()->standardIcon(QStyle::SP_TitleBarMenuButton)));
  QMenu* menu = new QMenu(this);
  connect(menu->addAction("show"), &QAction::triggered, this,
          &FramelessWindows::show);
  connect(menu->addAction("quit"), &QAction::triggered, qApp,
          &QApplication::quit);
  trayIcon->setContextMenu(menu);
  trayIcon->setToolTip("FramelessWindows");

  connect(trayIcon.get(), &QSystemTrayIcon::activated,
          [this](QSystemTrayIcon::ActivationReason reason) {
            switch (reason) {
              case QSystemTrayIcon::Unknown:
                break;
              case QSystemTrayIcon::Context:
                qDebug() << "context";
                break;
              case QSystemTrayIcon::DoubleClick:
                break;
              case QSystemTrayIcon::Trigger: {
                this->raise();
                this->activateWindow();
                this->show();

#ifdef Q_OS_WIN
                HWND setToHwnd = (HWND)winId();
                HWND hForgroundWnd = GetForegroundWindow();
                DWORD dwForeID =
                    ::GetWindowThreadProcessId(hForgroundWnd, NULL);
                DWORD dwCurID = ::GetCurrentThreadId();

                ::AttachThreadInput(dwCurID, dwForeID, TRUE);
                ::ShowWindow(setToHwnd, max_min_count > 0 ? SW_SHOWMAXIMIZED
                                                          : SW_SHOWNORMAL);
                ::SetForegroundWindow(setToHwnd);
                ::AttachThreadInput(dwCurID, dwForeID, FALSE);
#endif  // Q_WIN

              } break;
              case QSystemTrayIcon::MiddleClick:
                break;
              default:
                break;
            }
          });

  trayIcon->show();
  /* ui.label->setPixmap(
       QPixmap(QString::fromLocal8Bit("./Î¢ÐÅÍ¼Æ¬_20210320114323.png")));*/

  DPIMonitor::GetInstance()->setOriginalWindowSize(QSize(600, 400));
  REGISTERDPIOBJ()
  ("FramelessWindows", [this](double scale) { formInit(scale); });
}

bool FramelessWindows::nativeEvent(const QByteArray& eventType, void* message,
                                   long* result) {
#ifdef Q_OS_WIN
  if (eventType != "windows_generic_MSG") return false;

  MSG* msg = static_cast<MSG*>(message);

  switch (msg->message) {
    case WM_NCCALCSIZE: {
      *result = 0;
      return true;
    }

    case WM_NCHITTEST: {
      int x = GET_X_LPARAM(msg->lParam);
      int y = GET_Y_LPARAM(msg->lParam);

      QPoint pt = mapFromGlobal(QPoint(x, y));
      *result = calculateBorder(pt);
      if (*result == HTCLIENT) {
        QWidget* tempWidget = this->childAt(pt);
        if (tempWidget == ui.widget_2) {  // ±êÌâÀ¸
          *result = HTCAPTION;
        }
      }
      return true;
    }

    case WM_POWERBROADCAST: {
      if (msg->wParam == PBT_APMSUSPEND) {
        this->showMinimized();
        qDebug() << "PBT_APMSUSPEND";
      } else if (msg->wParam == PBT_APMRESUMEAUTOMATIC) {
        this->raise();
        this->activateWindow();
        this->showNormal();
        qDebug() << "PBT_APMRESUMEAUTOMATIC";
      }
    } break;

    case WM_GETMINMAXINFO: {
      if (::IsZoomed(msg->hwnd)) {
        if (max_min_count == 0) {
          ++max_min_count;
        }

        RECT frame = {0, 0, 0, 0};
        AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0,
                                 GetDpiForWindow(msg->hwnd));
        frame.left = abs(frame.left);
        frame.top = abs(frame.bottom);
        this->setContentsMargins(frame.left, frame.top, frame.right,
                                 frame.bottom);
        *result =
            ::DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        return true;
      } else if (::IsIconic(msg->hwnd)) {
        if (max_min_count > 0) {
          ++max_min_count;
        }
      } else {
        if (max_min_count > 0) {
          max_min_count = 0;
        }
      }
      this->setContentsMargins(0, 0, 0, 0);
      *result =
          ::DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
      return false;

    } break;

    default:
      break;
  }

#endif

  return QWidget::nativeEvent(eventType, message, result);
}

LRESULT FramelessWindows::calculateBorder(const QPoint& pt) {
  if (::IsZoomed((HWND)this->winId())) {
    return HTCLIENT;
  }
  int cx = this->size().width();
  int cy = this->size().height();

  QRect rectTopLeft(0, 0, borderSize, borderSize);
  if (rectTopLeft.contains(pt)) {
    return HTTOPLEFT;
  }

  QRect rectLeft(0, borderSize, borderSize, cy - borderSize * 2);
  if (rectLeft.contains(pt)) {
    return HTLEFT;
  }

  QRect rectTopRight(cx - borderSize, 0, borderSize, borderSize);
  if (rectTopRight.contains(pt)) {
    return HTTOPRIGHT;
  }

  QRect rectRight(cx - borderSize, borderSize, borderSize, cy - borderSize * 2);
  if (rectRight.contains(pt)) {
    return HTRIGHT;
  }

  QRect rectTop(borderSize, 0, cx - borderSize * 2, borderSize);
  if (rectTop.contains(pt)) {
    return HTTOP;
  }

  QRect rectBottomLeft(0, cy - borderSize, borderSize, borderSize);
  if (rectBottomLeft.contains(pt)) {
    return HTBOTTOMLEFT;
  }

  QRect rectBottomRight(cx - borderSize, cy - borderSize, borderSize,
                        borderSize);
  if (rectBottomRight.contains(pt)) {
    return HTBOTTOMRIGHT;
  }

  QRect rectBottom(borderSize, cy - borderSize, cx - borderSize * 2,
                   borderSize);
  if (rectBottom.contains(pt)) {
    return HTBOTTOM;
  }

  return HTCLIENT;
}

void FramelessWindows::closeEvent(QCloseEvent* event) {
  if (trayIcon->isVisible()) {
    /*QMessageBox::information(this, tr("Systray"),
        tr("The program will keep running in the "
            "system tray. To terminate the program, "
            "choose <b>Quit</b> in the context menu "
            "of the system tray entry."));*/
    hide();
    event->ignore();
  }
}

void FramelessWindows::formInit(double scale) {
  this->setMinimumSize(SCALEUP(600 * scale), SCALEUP(400 * scale));
  ui.bt_close->setFixedSize(SCALEUP(100 * scale), SCALEUP(30 * scale));
  ui.bt_maxmize->setFixedSize(SCALEUP(100 * scale), SCALEUP(30 * scale));
  ui.bt_minmize->setFixedSize(SCALEUP(100 * scale), SCALEUP(30 * scale));
  ui.comboBox->setFixedSize(SCALEUP(200 * scale), SCALEUP(30 * scale));
  ui.widget_2->setFixedHeight(SCALEUP(30 * scale));

  ui.label->setStyleSheet(
      QString::fromUtf8(
          "font: %1px \"\345\276\256\350\275\257\351\233\205\351\273\221\";")
          .arg(SCALEUP(20 * scale)));

  ui.label_2->setStyleSheet(
      QString::fromUtf8(
          "font: %1px \"\345\276\256\350\275\257\351\233\205\351\273\221\";")
          .arg(SCALEUP(33 * scale)));

  QFont pushButtonFont = ui.bt_close->font();
  pushButtonFont.setPointSize(SCALEUP(9 * scale));
  ui.bt_close->setFont(pushButtonFont);
  ui.bt_maxmize->setFont(pushButtonFont);
  ui.bt_minmize->setFont(pushButtonFont);

  QFont comboBoxFont = ui.comboBox->font();
  comboBoxFont.setPointSize(SCALEUP(9 * scale));
  ui.comboBox->setFont(comboBoxFont);

  borderSize = SCALEUP(borderSize * scale);
  // ·ÀÖ¹ÍÐÅÌÍ¼±êÄ£ºý
  trayIcon->setIcon(qApp->style()->standardIcon(QStyle::SP_TitleBarMenuButton));
}
