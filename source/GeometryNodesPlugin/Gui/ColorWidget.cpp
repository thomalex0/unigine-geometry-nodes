#include "ColorWidget.h"

#include <QColorDialog>

namespace GeometryNodes
{
	ColorWidget::ColorWidget(QWidget* parent)
		: QPushButton(parent)
	{
		connect(this, &QPushButton::clicked, this, &ColorWidget::changeColor);
	}

	void ColorWidget::changeColor()
	{
		QColor newColor = QColorDialog::getColor(color_, parentWidget());
		if (newColor.isValid() && newColor != color_)
		{
			setColor(newColor);
			emit valueChanged(color_);
		}
	}

	void ColorWidget::setColor(const QColor& c)
	{
		color_ = c;
		setStyleSheet("background-color: " + color_.name());
	}
} // namespace GeometryNodes