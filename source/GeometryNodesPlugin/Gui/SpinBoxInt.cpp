#include "SpinBoxInt.h"

#include <QIntValidator>

namespace GeometryNodes
{

	SpinBoxInt::SpinBoxInt(QWidget* parent)
		: AbstractSpinBox(parent)
	{
		prepareCacheValue(QString::number(value_));
		valueDeltaMultiplier_ = 1.0f;
	}

	void SpinBoxInt::setRange(int min, int max)
	{
		min_ = min;
		max_ = max;

		setValue(value_);
	}

	void SpinBoxInt::setValue(int v, bool supress)
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

	void SpinBoxInt::fromHandle()
	{
		setValue(lastValue_ + valueDelta_);
	}

	void SpinBoxInt::fromHandleSingle(double x)
	{
		setValue(value_ + x * valueDeltaMultiplier_);
	}

	void SpinBoxInt::recordLastValue()
	{
		lastValue_ = value_;
		valueDelta_ = 0;
	}

	QValidator* SpinBoxInt::createValidator()
	{
		return new QIntValidator(this);
	}

	void SpinBoxInt::editApply(const QString& val)
	{
		setValue(val.toInt());
	}
} // namespace GeometryNodes