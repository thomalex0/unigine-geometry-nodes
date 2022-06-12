#include "TripleSpinBox.h"

#include <QVBoxLayout>

namespace GeometryNodes
{
	TripleSpinBox::TripleSpinBox(QWidget* parent)
		: QWidget(parent)
	{
		auto main_layout = new QVBoxLayout(this);
		main_layout->setContentsMargins(0, 0, 0, 0);
		main_layout->setSpacing(1);
		setMaximumHeight(64);

		x_ = new SpinBoxDouble(this);
		y_ = new SpinBoxDouble(this);
		z_ = new SpinBoxDouble(this);
		main_layout->addWidget(x_);
		main_layout->addWidget(y_);
		main_layout->addWidget(z_);

		connect(x_, &SpinBoxDouble::valueChanged,
			this, &TripleSpinBox::onChanged);
		connect(y_, &SpinBoxDouble::valueChanged,
			this, &TripleSpinBox::onChanged);
		connect(z_, &SpinBoxDouble::valueChanged,
			this, &TripleSpinBox::onChanged);

		setTabOrder(x_, y_);
		setTabOrder(y_, z_);
	}

	TripleSpinBox::~TripleSpinBox()
	{
		disconnect(x_, &SpinBoxDouble::valueChanged,
			this, &TripleSpinBox::onChanged);
		disconnect(y_, &SpinBoxDouble::valueChanged,
			this, &TripleSpinBox::onChanged);
		disconnect(z_, &SpinBoxDouble::valueChanged,
			this, &TripleSpinBox::onChanged);
		x_->deleteLater();
		y_->deleteLater();
		z_->deleteLater();
	}

	void TripleSpinBox::setValue(Unigine::Math::vec3 v, bool supress)
	{
		x_->setValue(v.x, supress);
		y_->setValue(v.y, supress);
		z_->setValue(v.z, supress);

		if (!supress)
		{
			emit valueChanged(getValue());
		}
	}

	Unigine::Math::vec3 TripleSpinBox::getValue()
	{
		return Unigine::Math::vec3(x_->getValue(), y_->getValue(), z_->getValue());
	}

	void TripleSpinBox::setRange(double min, double max)
	{
		x_->setRange(min, max);
		y_->setRange(min, max);
		z_->setRange(min, max);
	}

	void TripleSpinBox::onChanged()
	{
		emit valueChanged(getValue());
	}

	void TripleSpinBox::setWidgetMinimumHeight(int h)
	{
		x_->setMinimumHeight(h);
		y_->setMinimumHeight(h);
		z_->setMinimumHeight(h);
	}
} // namespace GeometryNodes