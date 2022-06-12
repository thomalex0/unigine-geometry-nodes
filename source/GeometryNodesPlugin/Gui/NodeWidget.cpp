#include "NodeWidget.h"

//#include <UnigineLog.h>
//#include <UnigineVector.h>
#include <UnigineNode.h>
#include <UnigineWorld.h>
//
#include <QMouseEvent>
#include <QPainter>
//#include <QCursor>
//#include <QApplication>
#include <QMimeData>
//#include <QByteArray>
//#include <QDataStream>

namespace GeometryNodes
{
	const int TEXT_MARGIN{ 6 };
	const int ICON_MARGIN{ 4 };
	const QColor COLOR_BOTTOM{ "#494949" };
	const QColor COLOR_HOVER{ "#545454" };
	const QColor COLOR_TEXT{ "#C2C5CC" };

	NodeWidget::NodeWidget(QWidget* parent)
		: QWidget(parent)
		, update_pixmap_{ QPixmap(tr(":/bgn/icon_update.png")) }
	{
		setMinimumHeight(20);
		setAcceptDrops(true);
	}

	void NodeWidget::paintEvent(QPaintEvent* event)
	{
		Q_UNUSED(event);

		QPainter painter(this);

		const int w = width();
		const int h = height();

		painter.fillRect(0, 0, w, h, hovered ? COLOR_HOVER : COLOR_BOTTOM);

		if (node_ > 0)
		{
			QRect update_rect = QRect(w - update_pixmap_.width() - ICON_MARGIN, 0, update_pixmap_.width() + ICON_MARGIN, h);
			painter.fillRect(update_rect, COLOR_BOTTOM);
			painter.drawPixmap(update_rect.x() + ICON_MARGIN / 2, h / 2 - update_pixmap_.height() / 2, update_pixmap_);
		}

		painter.setPen(COLOR_TEXT);

		draw_text(painter, text_, Qt::AlignCenter);
	}

	void NodeWidget::enterEvent(QEvent* event)
	{
		hovered = true;
		update();
	}

	void NodeWidget::leaveEvent(QEvent* event)
	{
		hovered = false;
		update();
	}

	void NodeWidget::dragEnterEvent(QDragEnterEvent* event)
	{
		//for (auto i : event->mimeData()->formats())
		//{
		//	// application/unigine.hierarchy.list
		//	Unigine::Log::message("%s\n", i.toUtf8().constData());
		//}

		if (event->mimeData()->hasFormat(tr("application/unigine.hierarchy.list")))
		{
			setCursor(Qt::CursorShape::DragLinkCursor);
			event->acceptProposedAction();
		}
		else setCursor(Qt::CursorShape::CrossCursor);
	}
	void NodeWidget::dragLeaveEvent(QDragLeaveEvent* event)
	{
		unsetCursor();
	}

	void NodeWidget::dropEvent(QDropEvent* event)
	{
		auto format = tr("application/unigine.hierarchy.list");
		const QMimeData* mime = event->mimeData();
		if (mime && mime->hasFormat(format))
		{
			Unigine::Vector<int> ids;

			QByteArray encodedData = mime->data(format);
			QDataStream stream(&encodedData, QIODevice::ReadOnly);
			ids.allocate(encodedData.size() / sizeof(int));

			while (!stream.atEnd())
			{
				int id;
				stream >> id;
				ids.append(id);
			}

			if (ids.size() == 1)
			{
				setNodeId(ids[0]);
			}
		}
		unsetCursor();
	}

	void NodeWidget::mousePressEvent(QMouseEvent* event)
	{
		if (!(event->buttons() & Qt::LeftButton))
		{
			event->ignore();
			return;
		}

		if (node_ > 0)
		{
			QRect update_rect = QRect(width() - update_pixmap_.width() - ICON_MARGIN, 0, update_pixmap_.width() + ICON_MARGIN, height());
			if (update_rect.contains(event->x(), event->y()))
			{
				emit valueChanged(node_);
			}
		}

		QWidget::mousePressEvent(event);
	}

	void NodeWidget::setNodeId(int id)
	{
		if (id == -1 || id == 0) { return; }

		const Unigine::NodePtr node = Unigine::World::getNodeById(id);
		if (node)
		{
			node_ = id;
			updateText(tr("%1 (%2)").arg(node->getName(), node->getTypeName()));
			emit valueChanged(id);
		}
	}

	void NodeWidget::updateText(const QString& text)
	{
		text_.setText(text);
		text_.prepare(QTransform(), font());
	}

	void NodeWidget::draw_text(QPainter& painter, const QStaticText& text, int align)
	{
		const QSize w_size = size();
		const QSize t_size = text.size().toSize();

		const qreal x = (align == Qt::AlignCenter)
			? qMax(TEXT_MARGIN, w_size.width() / 2 - t_size.width() / 2)
			: TEXT_MARGIN;

		const qreal y = qAbs(t_size.height() - w_size.height()) * 0.5 + 1;

		painter.drawStaticText(x, y, text);
	}
} // namespace GeometryNodes