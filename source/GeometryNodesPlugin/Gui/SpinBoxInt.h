#pragma once

#include "AbstractSpinBox.h"

namespace GeometryNodes
{
	class SpinBoxInt final : public AbstractSpinBox
	{
		Q_OBJECT
			Q_DISABLE_COPY(SpinBoxInt)

	public:
		explicit SpinBoxInt(QWidget* parent);

		void setRange(int min, int max);

		int getMin() const { return min_; }
		void setMin(int min) { setRange(min, max_); }

		int getMax() const { return max_; }
		void setMax(int max) { setRange(min_, max); }

		int getValue() const { return value_; }
		void setValue(int v, bool supress = false);

	signals:
		void valueChanged(int v);

	private:
		QValidator* createValidator() override;
		void editApply(const QString& data) override;
		void fromHandle() override;
		void fromHandleSingle(double x) override;
		void recordLastValue() override;

	private:
		int min_{ -1000 };
		int max_{ 1000 };
		int value_{ 0 };
		int lastValue_{ 0 };
	};

} // namespace GeometryNodes