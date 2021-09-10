#include "FramelessWindows.h"

#include <qapplication.h>
#include <qdebug.h>
#include <qmessagebox.h>
#include <qoperatingsystemversion.h>
#include <qscreen.h>

#include <QWindowStateChangeEvent>

#ifdef Q_OS_WIN

#include <dwmapi.h>
#include <tchar.h>
#include <windowsx.h>

#pragma comment(lib, "Dwmapi.lib")

#endif  // Q_OS_WIN

#include <QCloseEvent>
#include <QMenu>

#include "DPIMonitor.h"

FramelessWindows::FramelessWindows(QWidget* parent)
    : QWidget(parent), oldState(Qt::WindowNoState) {
  ui.setupUi(this);

  setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
  setWindowTitle("FramelessWindows");

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

    // DWM_BLURBEHIND bb = {0};
    // HRGN hRgn = CreateRectRgn(0, 0, -1, -1);  //应用毛玻璃的矩形范围，
    ////参数(0,0,-1,-1)可以让整个窗口客户区变成透明的，而鼠标是可以捕获到透明的区域
    // bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    // bb.hRgnBlur = hRgn;
    // bb.fEnable = TRUE;
    // DwmEnableBlurBehindWindow(hwnd, &bb);
    // setAttribute(Qt::WA_TranslucentBackground);

    // HRESULT hr = S_OK;
    // HTHUMBNAIL thumbnail = NULL;
    // HWND hWndSrc = FindWindow(_T("FramelessWindows"), NULL);
    // hr = DwmRegisterThumbnail(hWndDst, hwnd, &thumbnail);
    // if (SUCCEEDED(hr)) {
    //  RECT dest;
    //  ::GetClientRect(hWndDst, &dest);

    //  DWM_THUMBNAIL_PROPERTIES dskThumbProps;
    //  dskThumbProps.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE |
    //                          DWM_TNP_SOURCECLIENTAREAONLY;
    //  dskThumbProps.fSourceClientAreaOnly = FALSE;
    //  dskThumbProps.fVisible = TRUE;
    //  dskThumbProps.opacity = (255 * 70) / 100;
    //  dskThumbProps.rcDestination = dest;

    //  hr = DwmUpdateThumbnailProperties(thumbnail, &dskThumbProps);
    //  if (SUCCEEDED(hr)) {
    //    // ...
    //    qDebug() << "DwmUpdateThumbnailProperties success!!!";
    //  }
    //}
  }

#endif  // Q_OS_WIN

  connect(ui.bt_close, &QPushButton::clicked, this, &FramelessWindows::close);
  connect(ui.bt_minmize, &QPushButton::clicked, this,
          &FramelessWindows::showMinimized);
  connect(ui.bt_maxmize, &QPushButton::clicked,
          [this] { isMaximized() ? showNormal() : showMaximized(); });

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

                if (isHidden() || isMinimized()) {
                  if (oldState & Qt::WindowMaximized) {
                    showMaximized();
                  } else {
                    showNormal();
                  }
                }

              } break;
              case QSystemTrayIcon::MiddleClick:
                break;
              default:
                break;
            }
          });

  //trayIcon->show();

  DPIMonitor::GetInstance()->setOriginalWindowSize(QSize(600, 400));
  REGISTERDPIOBJ()
  ("FramelessWindows", [this](double scale) { formInit(scale); });
}

FramelessWindows::~FramelessWindows() {
  if (hWndDst) {
    DwmUnregisterThumbnail(hWndDst);
  }
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
        if (tempWidget == ui.widget_2) {  // 标题栏
          *result = HTCAPTION;
        }
      }
      return true;
    }

    case WM_POWERBROADCAST: {
      static bool is_hide;
      static Qt::WindowStates current_state;
      if (msg->wParam == PBT_APMSUSPEND) {
        is_hide = this->isHidden();
        if (!is_hide) {
          current_state = this->windowState();
          this->showMinimized();
        }
        qDebug() << "PBT_APMSUSPEND";
      } else if (msg->wParam == PBT_APMRESUMEAUTOMATIC) {
        if (!is_hide) {
          this->raise();
          this->activateWindow();
          if (current_state & Qt::WindowMaximized) {
            this->showMaximized();
          } else if (current_state == Qt::WindowMinimized) {
            this->showMinimized();
          } else {
            this->showNormal();
          }
        }
        qDebug() << "PBT_APMRESUMEAUTOMATIC";
      }
    } break;

    case WM_GETMINMAXINFO: {
      if (::IsZoomed(msg->hwnd)) {
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
    setWindowState(Qt::WindowMinimized);
    hide();
    event->ignore();
  }
}

void FramelessWindows::changeEvent(QEvent* ev) {
  if (ev->type() == QEvent::WindowStateChange) {
    oldState = static_cast<QWindowStateChangeEvent*>(ev)->oldState();
  }
}

void FramelessWindows::formInit(double scale) {
  this->setMinimumSize(SCALEUP(600 * scale), SCALEUP(400 * scale));

  ui.widget_2->setStyleSheet(
      QString::fromUtf8(
          "QPushButton#bt_close,#bt_maxmize,#bt_minmize {\n"
          "background-color: lightyellow;\n"
          "border-style: outset;\n"
          "border-width: %2px;\n"
          "border-radius: %5px;\n"
          "border-color: beige;\n"
          "font: bold %6px;\n"
          "min-width: %3em;\n"
          "padding: %4px;\n"
          "min-height:%1em;\n"
          "}\n"
          "QPushButton#bt_close:pressed,#bt_maxmize:pressed,#bt_minmize:"
          "pressed {\n"
          "background-color: rgb(224, 0, 0);\n"
          "border-style: inset;\n"
          "color:yellow;\n"
          "}\n"
          "QPushButton#bt_close:hover,#bt_maxmize:hover,#bt_minmize:hover{\n"
          "color: lightblue;\n"
          "}")
          .arg(SCALEUP(1 * scale))
          .arg(SCALEUP(2 * scale))
          .arg(SCALEUP(5 * scale))
          .arg(SCALEUP(6 * scale))
          .arg(SCALEUP(10 * scale))
          .arg(SCALEUP(14 * scale)));

  ui.comboBox->setStyleSheet(
      QString::fromUtf8(
          "QComboBox{\n"
          "min-height:%2em;\n"
          "max-width:%5em;\n"
          "background-color:lightyellow;\n"
          "border:none;\n"
          "background:transparent;\n"
          "color:#000;\n"
          "font: bold italic %6px \"Microsoft YaHei\";\n"
          "}\n"
          "QComboBox::drop-down{\n"
          "subcontrol-origin: padding;\n"
          "subcontrol-position: top right;\n"
          "width: %6px;\n"
          "padding-right:%4px;\n"
          "border:none;\n"
          "}\n"
          "QComboBox::down-arrow{\n"
          "image:url(:/FramelessWindows/images/down.svg);\n"
          "width: %6px;\n"
          "height:%6px;\n"
          "}\n"
          "QComboBox QAbstractItemView {\n"
          "outline:0px solid gray;\n"
          "border: %1px solid darkgray;\n"
          "background-color:yellowgreen;\n"
          "selection-background-color: transparent;\n"
          "}\n"
          "QComboBox QAbstractItemView::item {\n"
          "height:%7px;\n"
          "}\n"
          "QComboBox QAbstractItemView::item:hover {\n"
          "color:#fff;\n"
          "}\n"
          "QComboBox QAbstractItemView::item:selected {\n"
          "color:#fff;\n"
          "}\n"
          "QComboBox QAbstractScrollArea QScrollBar:vertical {\n"
          "width: %5px;\n"
          "border-radius: %3px;\n"
          "background-color: #d0d2d4;\n"
          "}\n"
          "QComboBox QAbstractScrollArea QScrollBar::handle:vertical {\n"
          "width: %5px;\n"
          "border-radius: %3px;\n"
          "background: rgb(160,160,160);\n"
          "}\n"
          "QComboBox QAbstractScrollArea QScrollBar::handle:vertical:hover {\n"
          "background: rgb(90, 91, 93);\n"
          "}")
          .arg(SCALEUP(1 * scale))       // 1
          .arg(SCALEUP(2 * scale))       // 2
          .arg(SCALEUP(10 * scale) / 2)  // 3
          .arg(SCALEUP(6 * scale))       // 4
          .arg(SCALEUP(10 * scale))      // 5
          .arg(SCALEUP(15 * scale))      // 6
          .arg(SCALEUP(25 * scale))      // 7
  );

  ui.label->setStyleSheet(
      QString::fromUtf8(
          "font: %1px \"\345\276\256\350\275\257\351\233\205\351\273\221\";")
          .arg(SCALEUP(20 * scale)));

  ui.label_2->setStyleSheet(
      QString::fromUtf8(
          "font: %1px \"\345\276\256\350\275\257\351\233\205\351\273\221\";")
          .arg(SCALEUP(33 * scale)));

  // 防止托盘图标模糊
  trayIcon->setIcon(qApp->style()->standardIcon(QStyle::SP_TitleBarMenuButton));
}
