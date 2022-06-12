#include "ImageWidget.h"

//#include <UnigineLog.h>
//#include <UnigineVector.h>
#include <UnigineFileSystem.h>
//#include <UnigineImage.h>
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

	ImageWidget::ImageWidget(QWidget* parent)
		: QWidget(parent)
		, update_pixmap_{ QPixmap(tr(":/bgn/icon_update.png")) }
	{
		setMinimumHeight(20);
		setAcceptDrops(true);
	}

	void ImageWidget::paintEvent(QPaintEvent* event)
	{
		Q_UNUSED(event);

		QPainter painter(this);

		const int w = width();
		const int h = height();

		painter.fillRect(0, 0, w, h, hovered ? COLOR_HOVER : COLOR_BOTTOM);

		if (!asset_.isEmpty())
		{
			QRect update_rect = QRect(w - update_pixmap_.width() - ICON_MARGIN, 0, update_pixmap_.width() + ICON_MARGIN, h);
			painter.fillRect(update_rect, COLOR_BOTTOM);
			painter.drawPixmap(update_rect.x() + ICON_MARGIN / 2, h / 2 - update_pixmap_.height() / 2, update_pixmap_);
		}

		painter.setPen(COLOR_TEXT);

		draw_text(painter, text_, Qt::AlignCenter);
	}

	void ImageWidget::enterEvent(QEvent* event)
	{
		hovered = true;
		update();
	}

	void ImageWidget::leaveEvent(QEvent* event)
	{
		hovered = false;
		update();
	}

	const QString format = "application/unigine.assets.list.assets";

	Unigine::UGUID decode(const QMimeData* mime)
	{
		if (mime && mime->hasFormat(format))
		{
			Unigine::Vector<Unigine::UGUID> guids;

			QByteArray encodedData = mime->data(format);
			QDataStream stream(&encodedData, QIODevice::ReadOnly);
			guids.allocate(encodedData.size() / sizeof(int));

			while (!stream.atEnd())
			{
				QString guid;
				stream >> guid;
				guids.append(Unigine::UGUID(guid.toUtf8().constData()));
			}

			if (guids.size() == 1)
			{
				return guids[0];
			}
		}
		return Unigine::UGUID();
	}

	bool isSupported(Unigine::UGUID guid)
	{
		if (!guid.isValid() || guid.isEmpty())
		{
			return false;
		}

		Unigine::String path = Unigine::FileSystem::getVirtualPath(guid);

		if (Unigine::Image::isSupported(path))
		{
			return true;
		}
		return false;
	}

	void ImageWidget::dragEnterEvent(QDragEnterEvent* event)
	{
		/*for (auto i : event->mimeData()->formats())
		{
			// application/unigine.assets.list
			//application/unigine.assets.list.assets
			//application/unigine.assets.list.runtimes
			Unigine::Log::message("%s\n", i.toUtf8().constData());
		}*/

		if (isSupported(decode(event->mimeData())))
		{
			setCursor(Qt::CursorShape::DragLinkCursor);
			event->acceptProposedAction();
		}
		else setCursor(Qt::CursorShape::CrossCursor);
	}
	void ImageWidget::dragLeaveEvent(QDragLeaveEvent* event)
	{
		unsetCursor();
	}

	void ImageWidget::dropEvent(QDropEvent* event)
	{
		Unigine::UGUID guid = decode(event->mimeData());
		setAsset(guid);
		unsetCursor();
	}

	void ImageWidget::mousePressEvent(QMouseEvent* event)
	{
		if (!(event->buttons() & Qt::LeftButton))
		{
			event->ignore();
			return;
		}

		if (!asset_.isEmpty())
		{
			QRect update_rect = QRect(width() - update_pixmap_.width() - ICON_MARGIN, 0, update_pixmap_.width() + ICON_MARGIN, height());
			if (update_rect.contains(event->x(), event->y()))
			{
				emit valueChanged(asset_);
			}
		}

		QWidget::mousePressEvent(event);
	}

	void ImageWidget::setAsset(Unigine::UGUID guid)
	{
		if (isSupported(guid))
		{
			asset_ = guid;
			Unigine::String path = Unigine::FileSystem::getVirtualPath(guid);
			updateText(tr(path.basename().get()));
			emit valueChanged(guid);
		}
	}

	void ImageWidget::updateText(const QString& text)
	{
		text_.setText(text);
		text_.prepare(QTransform(), font());
	}

	void ImageWidget::draw_text(QPainter& painter, const QStaticText& text, int align)
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