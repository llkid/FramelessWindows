#include "DPIMonitor.h"

#include <ShellScalingApi.h>
#include <qapplication.h>
#include <qdebug.h>
#include <qmutex.h>
#include <qscreen.h>

QScopedPointer<DPIMonitor> DPIMonitor::self;

DPIMonitor::DPIMonitor(QObject* parent)
    : QObject(parent),
      originalWindowSize(0, 0),
      maxScalingFactor(1.75),
      turnOnDpiScaling(true) {
  connect(qApp->primaryScreen(), &QScreen::logicalDotsPerInchChanged,
          [this](qreal dpi) {
            if (turnOnDpiScaling) {
              double scale = dpi / 96;
              if (scale > maxScalingFactor) {
                scale = maxScalingFactor;
              }

              auto it = widgetsScaltor.constBegin();
              while (it != widgetsScaltor.constEnd()) {
                it.value()(scale);
                ++it;
              }
            }
          });

  connect(qApp->primaryScreen(), &QScreen::availableGeometryChanged,
          [this](const QRect& geometry) {
            double factor1 = geometry.width() / originalWindowSize.width();
            double factor2 = geometry.height() / originalWindowSize.height();
            maxScalingFactor = qMin(factor1, factor2);
            maxScalingFactor = qMax(maxScalingFactor, 1.75);
          });
}

DPIMonitor::~DPIMonitor() { widgetsScaltor.clear(); }

DPIMonitor* DPIMonitor::GetInstance() {
  if (self.isNull()) {
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    if (self.isNull()) {
      self.reset(new DPIMonitor);
    }
  }
  return self.get();
}

void DPIMonitor::registMonitoredObj(QString signature,
                                    std::function<void(double)>&& scaltor) {
  widgetsScaltor.insert(signature, std::move(scaltor));
  widgetsScaltor.value(signature)(qApp->primaryScreen()->logicalDotsPerInch() /
                                  96);
}

void DPIMonitor::unregistMonitoredObj(QString signature) {
  widgetsScaltor.remove(signature);
}

QSize DPIMonitor::getOriginalWindowSize() const { return originalWindowSize; }

void DPIMonitor::setOriginalWindowSize(const QSize& geometry) {
  this->originalWindowSize = geometry;

  const QRect availableGeometry = qApp->primaryScreen()->availableGeometry();
  double factor1 = availableGeometry.width() / originalWindowSize.width();
  double factor2 = availableGeometry.height() / originalWindowSize.height();
  maxScalingFactor = qMin(factor1, factor2);
  maxScalingFactor = qMax(maxScalingFactor, 1.75);
}

double DPIMonitor::getMaxScalingFactor() const { return maxScalingFactor; }

void DPIMonitor::setMaxScalingFactor(double factor) {
  this->maxScalingFactor = factor;
}

bool DPIMonitor::enableDpiScling() const { return turnOnDpiScaling; }

void DPIMonitor::setEnableDpiScling(bool enable) {
  this->turnOnDpiScaling = enable;
}
