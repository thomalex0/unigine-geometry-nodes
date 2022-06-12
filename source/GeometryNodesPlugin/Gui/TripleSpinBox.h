#pragma once
#include "SpinBoxDouble.h"
#include <UnigineMathLib.h>

//#include <QWidget>

namespace GeometryNodes
{
	class TripleSpinBox : public QWidget
	{
		Q_OBJECT
			Q_DISABLE_COPY(TripleSpinBox)

	public:
		explicit TripleSpinBox(QWidget* parent = nullptr);
		~TripleSpinBox();

		void setRange(double min, double max);
		Unigine::Math::vec3 getValue();
		void setValue(Unigine::Math::vec3 v, bool supress = false);
		void setWidgetMinimumHeight(int h);

	public slots:
		void onChanged();

	signals:
		void valueChanged(Unigine::Math::vec3 v);

	private:
		SpinBoxDouble* x_{ nullptr };
		SpinBoxDouble* y_{ nullptr };
		SpinBoxDouble* z_{ nullptr };
	};

} // namespace GeometryNodes