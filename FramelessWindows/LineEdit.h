#pragma once

#include <QLineEdit>

class LineEdit : public QLineEdit
{
	Q_OBJECT

public:
	LineEdit(QWidget *parent = nullptr);
	~LineEdit();

private:

	virtual void keyPressEvent(QKeyEvent* event) override;
	virtual void keyReleaseEvent(QKeyEvent* event) override;

private:
	QString content;
};
