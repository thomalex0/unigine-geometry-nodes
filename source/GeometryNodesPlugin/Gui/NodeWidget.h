#pragma once

#include <QWidget>
#include <QStaticText>
//#include <QLineEdit>
//#include <QValidator>
//#include <QPixmap>

namespace GeometryNodes
{

	class NodeWidget : public QWidget
	{
		Q_OBJECT
			Q_DISABLE_COPY(NodeWidget)

	public:
		explicit NodeWidget(QWidget* parent = nullptr);

		int getNodeId() const { return node_; }
		void setNodeId(int id);

	signals:
		void valueChanged(int n);

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

		int node_{ -1 };
		QStaticText text_{ "Default (drop nodes here)" };
		bool hovered{ false };
		QPixmap update_pixmap_;
	};

} // namespace GeometryNodes