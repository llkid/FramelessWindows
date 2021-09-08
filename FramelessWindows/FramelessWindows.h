#pragma once

#include <Windows.h>
#include <qsystemtrayicon.h>

#include <QtWidgets/QWidget>

#include "ui_FramelessWindows.h"

class FramelessWindows : public QWidget {
  Q_OBJECT

 public:
  FramelessWindows(QWidget* parent = Q_NULLPTR);
  ~FramelessWindows();

 private:
  Ui::FramelessWindowsClass ui;
  QScopedPointer<QSystemTrayIcon> trayIcon;

  bool nativeEvent(const QByteArray& eventType, void* message,
                   long* result) override;
  LRESULT calculateBorder(const QPoint& pt);
  void closeEvent(QCloseEvent* event) override;

 private:
  void formInit(double scale);

 private:
  int borderSize;
  int max_min_count;
  HWND hWndDst = NULL;
};
