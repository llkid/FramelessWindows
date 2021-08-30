#pragma once

#include <Windows.h>
#include <qsystemtrayicon.h>

#include <QtWidgets/QWidget>

#include "DPIMonitor.h"
#include "ui_FramelessWindows.h"

class FramelessWindows : public QWidget {
  Q_OBJECT

 public:
  FramelessWindows(QWidget* parent = Q_NULLPTR);

 private:
  Ui::FramelessWindowsClass ui;
  QSystemTrayIcon* trayIcon;

  bool nativeEvent(const QByteArray& eventType, void* message,
                   long* result) override;
  LRESULT calculateBorder(const QPoint& pt);
  void closeEvent(QCloseEvent* event) override;

  virtual void showEvent(QShowEvent* event) override;

 private:
  void formInit();

 private:
  QScopedPointer<DPIMonitor> sdm;
  int max_min_count;
};
