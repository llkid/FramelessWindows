#include "FramelessWindows.h"

#include <qdebug.h>
#include <qmessagebox.h>
#include <qoperatingsystemversion.h>
#include <qstyle.h>
#include <windowsx.h>

#include <QCloseEvent>
#include <QMenu>
#include <QtWinExtras/QtWin>

#include "DPIMonitor.h"

FramelessWindows::FramelessWindows(QWidget* parent)
    : QWidget(parent), max_min_count(0) {
  ui.setupUi(this);

  setWindowFlags(Qt::FramelessWindowHint);

  HWND hwnd = reinterpret_cast<HWND>(this->winId());
  DWORD style = GetWindowLong(hwnd, GWL_STYLE);
  SetWindowLongPtr(hwnd, GWL_STYLE,
                   style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);

  bool enabled = QtWin::isCompositionEnabled();
  if (enabled) {
    ::SetWindowLong(hwnd, GWL_STYLE,
                    style | WS_THICKFRAME | WS_CAPTION | WS_BORDER);
    QtWin::extendFrameIntoClientArea(this, 1, 1, 1, 1);
  }

  // connect(ui.pushButton, &QPushButton::clicked, qApp,
  // &QCoreApplication::quit);
  connect(ui.pushButton, &QPushButton::clicked, this, &FramelessWindows::close);

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

  trayIcon = new QSystemTrayIcon(
      QApplication::style()->standardIcon(QStyle::SP_TitleBarMenuButton), this);
  QMenu* menu = new QMenu(this);
  connect(menu->addAction("show"), &QAction::triggered, this,
          &FramelessWindows::show);
  connect(menu->addAction("quit"), &QAction::triggered, [this] {
    trayIcon->deleteLater();
    qApp->quit();
  });
  trayIcon->setContextMenu(menu);
  trayIcon->setToolTip("FramelessWindows");

  connect(trayIcon, &QSystemTrayIcon::activated,
          [this](QSystemTrayIcon::ActivationReason reason) {
            switch (reason) {
              case QSystemTrayIcon::Trigger:
              case QSystemTrayIcon::DoubleClick: {
                this->raise();
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
              default:;
            }
          });

  // trayIcon->show();
  /* ui.label->setPixmap(
       QPixmap(QString::fromLocal8Bit("./΢��ͼƬ_20210320114323.png")));*/

  DPIMonitor::GetInstance()->setOriginalWindowSize(QSize(600, 400));
  REGISTERDPIOBJ()
  ("FramelessWindows", [this](double scale) { formInit(scale); });
}

bool FramelessWindows::nativeEvent(const QByteArray& eventType, void* message,
                                   long* result) {
#ifdef Q_OS_WIN
  if (eventType != "windows_generic_MSG") return false;

  MSG* msg = static_cast<MSG*>(message);

  QWidget* widget = QWidget::find(reinterpret_cast<WId>(msg->hwnd));
  if (!widget) return false;

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
        if (tempWidget == NULL) {
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
        AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
        frame.left = abs(frame.left);
        frame.top = abs(frame.bottom);
        widget->setContentsMargins(frame.left, frame.top, frame.right,
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
      widget->setContentsMargins(0, 0, 0, 0);
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
  int borderSize = 4;
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
  ui.pushButton->setFixedSize(SCALEUP(100 * scale), SCALEUP(30 * scale));
  ui.comboBox->setFixedSize(SCALEUP(200 * scale), (30 * scale));

  ui.label->setStyleSheet(QString::fromUtf8(
      "font: %1px \"\345\276\256\350\275\257\351\233\205\351\273\221\";")
          .arg(SCALEUP(20 * scale))
  );

  ui.label_2->setStyleSheet(QString::fromUtf8(
      "font: %1px \"\345\276\256\350\275\257\351\233\205\351\273\221\";")
          .arg(SCALEUP(33 * scale))
  );

  QFont pushButtonFont = ui.pushButton->font();
  pushButtonFont.setPointSize(SCALEUP(9 * scale));
  ui.pushButton->setFont(pushButtonFont);

  QFont comboBoxFont = ui.comboBox->font();
  comboBoxFont.setPointSize(SCALEUP(9 * scale));
  ui.comboBox->setFont(comboBoxFont);
}
