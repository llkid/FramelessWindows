#pragma once

#include <qhash.h>
#include <qsize.h>

#include <QObject>

#include <functional>

class DPIMonitor : public QObject {
  Q_OBJECT

 public:
  ~DPIMonitor();
  static DPIMonitor* GetInstance();

 public:
  void registMonitoredObj(QString signature,
                          std::function<void(double)>&& scaltor);
  void unregistMonitoredObj(QString signature);

  QSize getOriginalWindowSize() const;
  void setOriginalWindowSize(const QSize& geometry);

  double getMaxScalingFactor() const;
  void setMaxScalingFactor(double factor);

 private:
  static QScopedPointer<DPIMonitor> self;
  QHash<QString, std::function<void(double)>> widgetsScaltor;
  QSize originalWindowSize;
  double maxScalingFactor;

  private:
  DPIMonitor(QObject* parent = nullptr);
  Q_DISABLE_COPY(DPIMonitor)
};

struct REGISTERDPIOBJ {
  void operator()(QString signature, std::function<void(double)>&& scaltor) {
    DPIMonitor::GetInstance()->registMonitoredObj(signature,
                                                  std::move(scaltor));
  }
};

struct UNREGISTERDPIOBJ {
  void operator()(QString signature, std::function<void(double)>&& scaltor) {
    DPIMonitor::GetInstance()->unregistMonitoredObj(signature);
  }
};

/*
 * @brief 获取临近最大偶数
 */
#define SCALEUP(param) \
  (int)(param) + ((int)(param) == param ? 0 : ((int)(param) / 2 ? 1 : 0))
