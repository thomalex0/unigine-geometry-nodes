#pragma once

#include <UnigineGUID.h>

#include <QWidget>
#include <QStaticText>
//#include <QLineEdit>
//#include <QValidator>
//#include <QPixmap>

namespace GeometryNodes
{
	class ImageWidget : public QWidget
	{
		Q_OBJECT
			Q_DISABLE_COPY(ImageWidget)

	public:
		explicit ImageWidget(QWidget* parent = nullptr);

		Unigine::UGUID getAsset() const { return asset_; }
		void setAsset(Unigine::UGUID guid);

	signals:
		void valueChanged(Unigine::UGUID n);

	protected:

		void paintEvent(QPaintEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void leaveEvent(QEvent* event) override;
		void enterEvent(QEvent* event) override;
		void dragEnterEvent(QDragEnterEvent* event) override;
		void dragLeaveEvent(QDragLeaveEvent* event) override;
		void dropEvent(QDropEvent* event) override;

		void updateText(const QString& text);

	private:
		void draw_text(QPainter& painter, const QStaticText& text, int align);

		Unigine::UGUID asset_{};
		QStaticText text_{ "Default (drop images here)" };
		bool hovered{ false };
		QPixmap update_pixmap_;
	};

} // namespace GeometryNodes