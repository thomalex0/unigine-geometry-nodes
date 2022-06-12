#pragma once

#include "AbstractSpinBox.h"

namespace GeometryNodes
{
	class SpinBoxDouble final : public AbstractSpinBox
	{
		Q_OBJECT
			Q_DISABLE_COPY(SpinBoxDouble)

	public:
		explicit SpinBoxDouble(QWidget* parent = nullptr);

		void setRange(double min, double max);

		double getMin() const { return min_; }
		void setMin(double min) { setRange(min, max_); }

		double getMax() const { return max_; }
		void setMax(double max) { setRange(min_, max); }

		double getValue() const { return value_; }
		void setValue(double v, bool supress = false);

	signals:
		void valueChanged(double v);

	private:
		QValidator* createValidator() override;
		void editApply(const QString& data) override;
		void fromHandle() override;
		void fromHandleSingle(double x) override;
		void recordLastValue() override;

	private:
		double min_{ -1000.0 };
		double max_{ 1000.0 };
		double value_{ 0.0 };
		double lastValue_{ 0.0 };
	};

} // namespace GeometryNodes