#include "AbstractSpinBox.h"

//#include <UnigineLog.h>

#include <QMouseEvent>
#include <QPainter>
//#include <QCursor>
#include <QApplication>
#include <QLineEdit>

namespace GeometryNodes
{
	const int TEXT_MARGIN{ 6 };
	const QColor COLOR_BOTTOM{ "#797979" };
	const QColor COLOR_HOVER{ "#545454" };
	const QColor COLOR_EDIT{ "#292a2b" };
	const QColor COLOR_TEXT{ "#C2C5CC" };

	const QString EDITOR_STYLE = "QLineEdit { border: none; }"
		"QLineEdit:focus { border: none; }";

	AbstractSpinBox::AbstractSpinBox(QWidget* parent)
		: QWidget(parent)
	{
		setFocusPolicy(Qt::StrongFocus);
		setCursor(Qt::CursorShape::SizeHorCursor);
		setMinimumHeight(20);
	}

	void AbstractSpinBox::paintEvent(QPaintEvent* event)
	{
		Q_UNUSED(event);

		QPainter painter(this);

		const int w = width();
		const int h = height();

		painter.fillRect(0, 0, w, h, hovered || is_spinbox_edit_ ? COLOR_HOVER : COLOR_BOTTOM);

		painter.setPen(COLOR_TEXT);

		draw_text(painter, cache_value_, Qt::AlignCenter);
	}

	void AbstractSpinBox::focusInEvent(QFocusEvent* event)
	{
		//if (!event->type() & QEvent::Type::MouseButtonPress) { edit_start(); }
		if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason) { edit_start(); }
		QWidget::focusInEvent(event);
	}

	void AbstractSpinBox::focusOutEvent(QFocusEvent* event)
	{
		edit_finish();
		QWidget::focusOutEvent(event);
	}

	void AbstractSpinBox::enterEvent(QEvent* event)
	{
		hovered = true;
		update();
	}

	void AbstractSpinBox::leaveEvent(QEvent* event)
	{
		hovered = false;
		update();
	}

	void AbstractSpinBox::mousePressEvent(QMouseEvent* event)
	{
		if (!(event->buttons() & Qt::LeftButton))
		{
			event->ignore();
			return;
		}
		lastPos_ = QCursor::pos();
		recordLastValue();
		QWidget::mousePressEvent(event);
	}

	void AbstractSpinBox::mouseMoveEvent(QMouseEvent* event)
	{
		if (!(event->buttons() & Qt::LeftButton))
		{
			event->ignore();
			return;
		}

		if (!is_spinbox_edit_)
		{
			is_spinbox_edit_ = true;
		}

		auto newPos = QCursor::pos();
		auto diff = newPos - lastPos_;
		double xd = diff.x() / 10.0;

		const auto mods = QApplication::keyboardModifiers();
		if (mods & Qt::ShiftModifier)
		{
			xd *= 4;
		}
		if (mods & Qt::ControlModifier)
		{
			xd /= 10;
		}
		valueDelta_ += xd;
		fromHandle();

		// workaround for rdc
		lastPos_ = newPos;

		// doesn't work via rdc
		QCursor::setPos(lastPos_);

		event->accept();
	}

	void AbstractSpinBox::mouseReleaseEvent(QMouseEvent* event)
	{
		if (!is_spinbox_edit_ && (event->button() == Qt::LeftButton))
		{
			edit_start();
		}

		is_spinbox_edit_ = false;
		QWidget::mouseReleaseEvent(event);
	}

	void AbstractSpinBox::keyPressEvent(QKeyEvent* event)
	{
		if (event->matches(QKeySequence::Cancel))
		{
			edit_finish(false);
			event->accept();
			return;
		}

		switch (event->key())
		{
		case Qt::Key_Enter:
		case Qt::Key_Return:
			edit_finish();
			event->accept();
			return;
		}

		if (editor_)
		{
			editor_->event(event);

			if (event->isAccepted())
			{
				return;
			}
		}
		else
		{
			if (event->key() == Qt::Key_Left) { fromHandleSingle(-1.0); }
			else if (event->key() == Qt::Key_Right) { fromHandleSingle(1.0); }
		}

		QWidget::keyPressEvent(event);
	}

	void AbstractSpinBox::keyReleaseEvent(QKeyEvent* event)
	{
		if (editor_)
		{
			editor_->event(event);

			if (event->isAccepted())
			{
				return;
			}
		}

		QWidget::keyReleaseEvent(event);
	}

	void AbstractSpinBox::resizeEvent(QResizeEvent* event)
	{
		Q_UNUSED(event)
			if (editor_)
			{
				editor_->setGeometry(rect());
			}
	}

	void AbstractSpinBox::prepareCacheValue(const QString& text)
	{
		cache_value_.setText(text);
		cache_value_.prepare(QTransform(), font());
	}

	void AbstractSpinBox::draw_text(QPainter& painter, const QStaticText& text, int align)
	{
		const QSize w_size = size();
		const QSize t_size = text.size().toSize();

		const qreal x = (align == Qt::AlignCenter)
			? qMax(TEXT_MARGIN, w_size.width() / 2 - t_size.width() / 2)
			: TEXT_MARGIN;

		const qreal y = qAbs(t_size.height() - w_size.height()) * 0.5 + 1;

		painter.drawStaticText(x, y, text);
	}

	void AbstractSpinBox::edit_start()
	{
		is_text_edit_ = true;

		editor_ = new QLineEdit(this);
		editor_->setStyleSheet(EDITOR_STYLE);
		editor_->setContextMenuPolicy(Qt::NoContextMenu);

		editor_->setGeometry(rect());
		editor_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

		editor_->setFrame(false);
		editor_->setFocusProxy(this);
		editor_->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
		editor_->setAcceptDrops(false);
		editor_->setValidator(createValidator());

		editor_->setText(cache_value_.text());
		editor_->selectAll();
		editor_->show();
	}

	void AbstractSpinBox::edit_finish(bool is_apply)
	{
		if (!is_text_edit_ || !editor_)
		{
			return;
		}

		is_text_edit_ = false;

		if (is_apply)
		{
			editApply(editor_->text());
		}

		delete editor_;
		editor_ = nullptr;
	}
} // namespace GeometryNodes