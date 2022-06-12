#pragma once

#include <QPushButton>
//#include <QColor>

namespace GeometryNodes
{
	class ColorWidget : public QPushButton
	{
		Q_OBJECT
	public:
		ColorWidget(QWidget* parent);

		void setColor(const QColor& color);
		const QColor& getColor() const { return color_; }

	public slots:
		void changeColor();

	signals:
		void valueChanged(QColor& color);

	private:
		QColor color_;
	};
} // namespace GeometryNodes