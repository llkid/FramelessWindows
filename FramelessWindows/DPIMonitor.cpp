#include "DPIMonitor.h"

#include <qdebug.h>
#include <qevent.h>
#include <qregularexpression.h>
#include <qscreen.h>
#include <qwidget.h>

DPIMonitor::DPIMonitor(QObject* parent) : QObject(parent) {}

DPIMonitor::~DPIMonitor() {
  if (parent_) {
    parent_->removeEventFilter(this);
    parent_ = nullptr;
  }
}

void DPIMonitor::setWidget(QWidget* parent) {
  if (!this->parent_) {
    this->parent_ = parent;

    static const auto scaler = [this](qreal scale) {
      QHash<QWidget*, QVariant>::const_iterator i1 = widgetsQss_.constBegin();
      while (i1 != widgetsQss_.constEnd()) {
        if (i1.value().toInt()) {
          auto font = i1.key()->font();
          font.setPointSize(i1.value().toInt() * scale);
          i1.key()->setFont(font);
        } else {
          i1.key()->setStyleSheet(getScaleQSS(i1.value().toString(), scale));
        }
        ++i1;
      }

      QHash<QWidget*, QSize>::const_iterator i2 =
          widgetsFixedSize_.constBegin();
      while (i2 != widgetsFixedSize_.constEnd()) {
        i2.key()->setFixedSize(i2.value() * scale);
        ++i2;
      }
    };

    connect(parent_->screen(), &QScreen::logicalDotsPerInchChanged,
            [this](qreal dpi) {
              qreal scale = dpi / 96;
              qDebug() << "Logical DPI:" << dpi << scale;

              scaler(scale);
            });
    parent_->installEventFilter(this);

    for (auto widget : parent_->findChildren<QWidget*>()) {
      if (widget) {
        auto qss = widget->styleSheet();
        widgetsQss_.insert(widget, qss.isEmpty()
                                       ? QVariant(widget->font().pointSize())
                                       : QVariant(qss));
        if (widget->sizePolicy() ==
            QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed)) {
          widgetsFixedSize_.insert(widget, widget->size());
          qDebug() << widget << widget->size();
        }
      }
    }

    qDebug() << widgetsQss_.size();
    scaler(parent_->screen()->logicalDotsPerInch() / 96);
  }
}

bool DPIMonitor::eventFilter(QObject* watched, QEvent* event) {
  if (watched == parent_) {
    // qDebug() << event->type();
  }

  return false;
}

QString DPIMonitor::getScaleQSS(QStringView strView, qreal dpi) {
  if (!strView.isEmpty()) {
    auto str = strView.toString();
    QRegularExpression re("(\\d+)(pt|px)");
    auto it = re.globalMatch(str, 0);
    while (it.hasNext()) {
      auto match = it.next();
      str.replace(match.captured(0), QString::number(static_cast<int>(
                                         match.captured(1).toDouble() * dpi)) +
                                         match.captured(2));
    }
    return str;
  }

  return QString();
}
