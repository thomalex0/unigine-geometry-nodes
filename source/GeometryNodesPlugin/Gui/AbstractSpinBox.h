#pragma once

#include <QWidget>
#include <QStaticText>
//#include <QValidator>
class QValidator;
class QLineEdit;

namespace GeometryNodes
{

	class AbstractSpinBox : public QWidget
	{
		Q_OBJECT
			Q_DISABLE_COPY(AbstractSpinBox)

	public:
		explicit AbstractSpinBox(QWidget* parent = nullptr);

	protected:
		virtual QValidator* createValidator() = 0;
		virtual void editApply(const QString& val) = 0;
		virtual void fromHandle() = 0;
		virtual void fromHandleSingle(double x) = 0;
		virtual void recordLastValue() = 0;

		void paintEvent(QPaintEvent* event) override;
		void focusInEvent(QFocusEvent* event) override;
		void focusOutEvent(QFocusEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void leaveEvent(QEvent* event) override;
		void enterEvent(QEvent* event) override;
		void keyPressEvent(QKeyEvent* event) override;
		void keyReleaseEvent(QKeyEvent* event) override;
		void resizeEvent(QResizeEvent* event) override;

		void prepareCacheValue(const QString& text);

		double valueDelta_{ 0 };
		double valueDeltaMultiplier_{ 1.0 };

	private:
		void draw_text(QPainter& painter, const QStaticText& text, int align);
		void edit_start();
		void edit_finish(bool is_apply = true);

		bool is_spinbox_edit_{ false };
		bool is_text_edit_{ false };

		QLineEdit* editor_{ nullptr };

		QStaticText cache_value_;
		QPoint lastPos_;
		bool hovered{ false };
	};

} // namespace GeometryNodes