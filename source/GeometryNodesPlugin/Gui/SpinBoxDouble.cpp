#include "SpinBoxDouble.h"

#include <QDoubleValidator>

namespace GeometryNodes
{

	SpinBoxDouble::SpinBoxDouble(QWidget* parent)
		: AbstractSpinBox(parent)
	{
		prepareCacheValue(QString::number(value_));
		valueDeltaMultiplier_ = 0.1;
	}

	void SpinBoxDouble::setRange(double min, double max)
	{
		min_ = min;
		max_ = max;

		setValue(value_);
	}

	void SpinBoxDouble::setValue(double v, bool supress)
	{
		v = qBound(min_, v, max_);

		if (v == value_)
		{
			return;
		}

		value_ = v;
		prepareCacheValue(QString::number(value_));

		update();
		if (!supress)
		{
			emit valueChanged(value_);
		}
	}

	void SpinBoxDouble::fromHandle()
	{
		setValue(lastValue_ + valueDelta_);
	}

	void SpinBoxDouble::fromHandleSingle(double x)
	{
		setValue(value_ + x * valueDeltaMultiplier_);
	}

	void SpinBoxDouble::recordLastValue()
	{
		lastValue_ = value_;
		valueDelta_ = 0;
	}

	QValidator* SpinBoxDouble::createValidator()
	{
		return new QDoubleValidator(this);
	}

	void SpinBoxDouble::editApply(const QString& val)
	{
		setValue(val.toDouble());
	}
} // namespace GeometryNodes