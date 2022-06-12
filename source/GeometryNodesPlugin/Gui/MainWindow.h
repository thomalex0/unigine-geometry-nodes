#pragma once

#include "../BlendAsset.h"

#include <QToolButton>

class QLabel;
class QFormLayout;
class QScrollArea;
class QPushButton;
class QMovie;

namespace GeometryNodes
{

	class LockButton : public QToolButton
	{
		Q_OBJECT

	public:
		explicit LockButton(QWidget* parent = nullptr);

		bool isLocked() const;
		void setLocked(bool v);

	signals:
		void toggled(bool v);

	protected:
		void mousePressEvent(QMouseEvent* event);

		QPixmap locked_pixmap_;
		QPixmap unlocked_pixmap_;
		bool locked_{};
	};

	class MainWindow final : public QWidget
	{
		Q_OBJECT
	public:
		MainWindow(QWidget* parent = nullptr);
		~MainWindow();

		void updateParamsGui(BlendAsset* comp = nullptr);
		void showUpdate();
		void hideUpdate();

	public slots:
		void onSelectionChanged();

	private:
		void setup_gui();
		void clear_params_gui();
		void assign_component();
		void update_current_component();
		void debounce_parameter_change(QString id, QVariant value);
		void parameter_change(QString id, QVariant value);

		//QTimer* debounce_timer;
		BlendAsset* currentBlend{};

		QScrollArea* scroll_area{};
		QFormLayout* params_layout{};
		QPushButton* assign_button{};
		QLabel* hint_label{};
		QLabel* update_label{};
		QMovie* update_movie{};
		LockButton* locker{};
	};

} // namespace GeometryNodes