#include "LineEdit.h"

#include <qdebug.h>
#include <QKeyEvent>

#include <chrono>
#include <atomic>

static std::chrono::steady_clock::time_point ellip;
static std::chrono::steady_clock::time_point ellip_between;
static bool manal = true;
static bool manal_between = true;

LineEdit::LineEdit(QWidget *parent)
	: QLineEdit(parent)
{
	setAttribute(Qt::WA_InputMethodEnabled, false);
}

LineEdit::~LineEdit()
{
}

void LineEdit::keyPressEvent(QKeyEvent* event)
{
	static std::atomic<int> cnt = 0;
	qDebug() << event->type();
	ellip = std::chrono::steady_clock::now();
	if (cnt == 0) {
		ellip_between = std::chrono::steady_clock::now();
	}

	if (++cnt == 2) {
		cnt = 0;
		auto cnt = std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::steady_clock::now() - ellip_between).count();
		qDebug() << cnt << "milliseconds";

		if (cnt > 300) {
			manal_between = true;
		}
		else {
			manal_between = false;
			content = this->text();
			//__super::keyPressEvent(event);
		}
	}
}

void LineEdit::keyReleaseEvent(QKeyEvent* event)
{
	qDebug() << event->type();
	auto dur = std::chrono::steady_clock::now() - ellip;
	auto cnt = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
	qDebug() << cnt << "milliseconds";
	if (cnt > 100) {
		manal = true;
	}
	else {
		manal = false;
	}

	if (!manal && !manal_between) {
		setText(content);
		//__super::keyReleaseEvent(event);
	}
}
