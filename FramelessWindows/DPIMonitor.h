#pragma once

#include <qhash.h>

#include <QObject>

class DPIMonitor : public QObject {
  Q_OBJECT

 public:
  DPIMonitor(QObject* parent = nullptr);
  ~DPIMonitor();

 public:
  void setWidget(QWidget* parent);

 private:
  virtual bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  static QString getScaleQSS(QStringView strView, qreal dpi);

 private:
  QWidget* parent_ = nullptr;
  QHash<QWidget*, QVariant> widgetsQss_;
  QHash<QWidget*, QSize> widgetsFixedSize_;
};
