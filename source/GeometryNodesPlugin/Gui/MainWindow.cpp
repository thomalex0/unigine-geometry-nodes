#include "MainWindow.h"
#include "SpinBoxInt.h"
#include "SpinBoxDouble.h"
#include "TripleSpinBox.h"
#include "NodeWidget.h"
#include "ImageWidget.h"
#include "ColorWidget.h"

//#include <UnigineVector.h>
#include <editor/Selection.h>
#include <editor/Selector.h>
#include <editor/ViewportManager.h>
#include <editor/WindowManager.h>
//
//#include <QHBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
//#include <QVariant>
//#include <QSizePolicy>
#include <QGroupBox>
#include <QScrollArea>
#include <QFormLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QMovie>

namespace GeometryNodes
{

	MainWindow::MainWindow(QWidget* parent) : QWidget{ parent }
		, update_movie{ new QMovie(tr(":/bgn/anim_update.gif"), {}, this) }
	{
		//Q_INIT_RESOURCE(bgn_resources);
		setWindowTitle("Geometry Nodes");
		setObjectName("GeometryNodesPluginView");
		setup_gui();
		hideUpdate();

		locker = new LockButton(this);
		Editor::WindowManager::addCornerWidget(this, locker);

		//debounce_timer = new QTimer(this);
		//debounce_timer->setSingleShot(true);

		onSelectionChanged();

		connect(Editor::Selection::instance(), &Editor::Selection::changed, this, &MainWindow::onSelectionChanged);
		connect(locker, &LockButton::toggled, this, [&](bool v)
		{
			if (!v)
				onSelectionChanged();
		});
	}

	MainWindow::~MainWindow()
	{
		clear_params_gui();

		disconnect(assign_button, nullptr, this, nullptr);
		//disconnect(debounce_timer, nullptr, this, nullptr);
		disconnect(Editor::Selection::instance(), nullptr, this, nullptr);

		delete assign_button;
		assign_button = nullptr;

		delete hint_label;
		hint_label = nullptr;

		delete update_label;
		update_label = nullptr;

		delete update_movie;
		update_movie = nullptr;

		delete params_layout;
		params_layout = nullptr;

		delete scroll_area;
		scroll_area = nullptr;

		delete locker;
		locker = nullptr;

		/*delete debounce_timer;
		debounce_timer = nullptr;*/
	}

	void MainWindow::setup_gui()
	{
		auto main_layout = new QVBoxLayout(this);
		main_layout->setContentsMargins(4, 4, 4, 4);

		auto controls_layout = new QVBoxLayout();
		controls_layout->setContentsMargins(4, 4, 4, 4);
		controls_layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

		hint_label = new QLabel(this);
		hint_label->setWordWrap(true);
		hint_label->setAlignment(Qt::AlignCenter);
		hint_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		controls_layout->addWidget(hint_label);

		assign_button = new QPushButton(this);
		connect(assign_button, &QPushButton::clicked,
			this, &MainWindow::assign_component);
		assign_button->setText(tr("Assign to selection"));
		controls_layout->addWidget(assign_button);
		assign_button->setMaximumWidth(assign_button->sizeHint().width() + 20);

		main_layout->addLayout(controls_layout);

		scroll_area = new QScrollArea(this);
		scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		scroll_area->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
		scroll_area->setWidgetResizable(true);
		main_layout->addWidget(scroll_area, 1);

		QGroupBox* params_widget = new QGroupBox("Parameters");
		params_layout = new QFormLayout(params_widget);
		params_layout->setContentsMargins(4, 4, 4, 4);

		params_widget->setLayout(params_layout);

		scroll_area->setWidget(params_widget);

		update_label = new QLabel(this);
		update_label->setMovie(update_movie);
		update_label->setAlignment(Qt::AlignRight);
		main_layout->addWidget(update_label);
	}

	void MainWindow::clear_params_gui()
	{
		while (params_layout->count() > 0)
		{
			auto item = params_layout->takeAt(0);
			if (auto widget = item->widget())
			{
				//disconnect()
				widget->deleteLater();
			}
			delete item;
		}
	}

	void MainWindow::updateParamsGui(BlendAsset* comp)
	{
		if (locker->isLocked()) return;
		clear_params_gui();

		update_current_component();

		if (!comp) comp = currentBlend;

		if (!comp || comp != currentBlend) return;

		if (comp->blend_asset.nullCheck())
		{
			hint_label->show();
			hint_label->setText(tr("The selected node doesn't have a *.blend asset assigned."));
			scroll_area->hide();
		}
		else
		{
			hint_label->hide();
			scroll_area->show();
		}

		for (const auto& param : comp->gnParams)
		{
			if (param.id == "Frame" && comp->skipFrame())
			{
				continue;
			};
			QWidget* w{};
			switch (param.type)
			{
			case GNParam::Type::BOOLEAN:
			{
				auto cb = new QCheckBox(params_layout->widget());
				cb->setChecked(param.value.toBool());
				connect(cb, &QCheckBox::stateChanged, this, [this, cb](int state)
				{
					debounce_parameter_change(cb->objectName(), QVariant(state != 0));
				});
				w = cb;
			}
			break;
			case GNParam::Type::OBJECT:
			case GNParam::Type::COLLECTION:
			{
				auto nw = new NodeWidget(params_layout->widget());
				nw->setNodeId(param.value.toInt());
				connect(nw, &NodeWidget::valueChanged, this, [this, nw](int v)
				{
					debounce_parameter_change(nw->objectName(), QVariant(v));
				});
				w = nw;
			}
			break;
			case GNParam::Type::IMAGE:
			{
				auto iw = new ImageWidget(params_layout->widget());
				Unigine::UGUID guid = Unigine::UGUID(param.value.toString().toUtf8().constData());
				if (guid.isValid() && !guid.isEmpty())
				{
					iw->setAsset(guid);
				}
				connect(iw, &ImageWidget::valueChanged, this, [this, iw](Unigine::UGUID v)
				{
					debounce_parameter_change(iw->objectName(), QVariant(tr(v.getString())));
				});
				w = iw;
			}
			break;
			case GNParam::Type::GEOMETRY:
				// not needed
				break;
			case GNParam::Type::INT:
			{
				auto sb = new SpinBoxInt(params_layout->widget());
				sb->setRange(param.min.i, param.max.i);
				sb->setValue(param.value.toInt(), true);
				connect(sb, &SpinBoxInt::valueChanged, this, [this, sb](int v)
				{
					debounce_parameter_change(sb->objectName(), QVariant(v));
				});
				w = sb;
			}
			break;
			case GNParam::Type::RGBA:
			{
				auto cw = new ColorWidget(params_layout->widget());
				QSequentialIterable it = param.value.value<QSequentialIterable>();
				cw->setColor(QColor::fromRgbF(it.at(0).toFloat(), it.at(1).toFloat(), it.at(2).toFloat(), it.at(3).toFloat()));
				connect(cw, &ColorWidget::valueChanged, this, [this, cw](QColor& c)
				{
					QList<qreal> l = { c.redF(), c.greenF(), c.blueF(), c.alphaF() };
					debounce_parameter_change(cw->objectName(), QVariant::fromValue(l));
				});
				w = cw;
			}
			break;
			case GNParam::Type::STRING:
			{
				auto le = new QLineEdit(param.value.toString().toUtf8().constData(), params_layout->widget());
				connect(le, &QLineEdit::textChanged, this, [this, le](const QString& s)
				{
					debounce_parameter_change(le->objectName(), QVariant(s.toUtf8().constData()));
				});
				w = le;
			}
			break;
			case GNParam::Type::VALUE:
			{
				auto sb = new SpinBoxDouble(params_layout->widget());
				sb->setRange(param.min.f, param.max.f);
				sb->setValue(param.value.toDouble(), true);
				connect(sb, &SpinBoxDouble::valueChanged, this, [this, sb](double v)
				{
					debounce_parameter_change(sb->objectName(), QVariant(v));
				});
				w = sb;
			}
			break;
			case GNParam::Type::VECTOR:
			{
				auto sb = new TripleSpinBox(params_layout->widget());
				sb->setRange(param.min.f, param.max.f);
				QSequentialIterable it = param.value.value<QSequentialIterable>();
				sb->setValue(Unigine::Math::vec3(it.at(0).toFloat(), it.at(1).toFloat(), it.at(2).toFloat()), true);
				connect(sb, &TripleSpinBox::valueChanged, this, [this, sb](Unigine::Math::vec3 v)
				{
					QList<float> l = { v.x, v.y, v.z };
					debounce_parameter_change(sb->objectName(), QVariant::fromValue(l));
				});
				w = sb;
			}
			break;
			}
			if (w != nullptr)
			{
				w->setObjectName(tr(param.id.get()));
				params_layout->addRow(tr(param.name.get()), w);
			}
		}
	}

	void MainWindow::debounce_parameter_change(QString id, QVariant value)
	{
		parameter_change(id, value);
		// TODO: figure out scopes
		/*debounce_timer->stop();
		disconnect(debounce_timer, nullptr, this, nullptr);
		connect(debounce_timer, &QTimer::timeout, this, [=]() {
			parameter_change(id, value);
		});
		debounce_timer->start(50);*/
	}

	void MainWindow::parameter_change(QString id, QVariant value)
	{
		if (currentBlend)
		{
			currentBlend->updateParameter(Unigine::String(id.toUtf8().constData()), value);
			showUpdate();
		}
	}

	void MainWindow::assign_component()
	{
		Editor::SelectorNodes* selector = Editor::Selection::getSelectorNodes();
		if (selector && selector->size())
		{
			for (const Unigine::NodePtr& node : selector->getNodes())
			{
				if (!Unigine::ComponentSystem::get()->getComponent<BlendAsset>(node))
				{
					Unigine::ComponentSystem::get()->addComponent<BlendAsset>(node);
				}
			}
			onSelectionChanged();
		}
		else
		{
			Unigine::NodePtr d = Unigine::NodeDummy::create();
			Unigine::ComponentSystem::get()->addComponent<BlendAsset>(d);
			Editor::ViewportManager::placeNode(d);
		}
	}

	void MainWindow::onSelectionChanged()
	{
		//if (!Unigine::World::isLoaded()) return;

		updateParamsGui();

		if (locker->isLocked()) return;

		if (currentBlend || !Unigine::World::isLoaded())
		{
			assign_button->hide();
			if (!Unigine::World::isLoaded())
			{
				hint_label->hide();
				scroll_area->hide();
			}
		}
		else
		{
			assign_button->show();
			hint_label->show();
			scroll_area->hide();
			Editor::SelectorNodes* selector = Editor::Selection::getSelectorNodes();
			if (selector && selector->size())
			{
				assign_button->setText(tr("Assign to Selection"));
				hint_label->setText(tr("The selected node doesn't have a BlendAsset property assigned."));
			}
			else
			{
				assign_button->setText(tr("Create New"));
				hint_label->setText(tr("No selected nodes."));
			}
		}

	}

	void MainWindow::update_current_component()
	{
		BlendAsset* comp{};
		Editor::SelectorNodes* selector = Editor::Selection::getSelectorNodes();
		if (selector && selector->size() == 1)
		{
			comp = Unigine::ComponentSystem::get()->getComponent<BlendAsset>(selector->getNodes()[0]);
		}
		currentBlend = comp;
	}

	void MainWindow::showUpdate()
	{
		update_label->show();
		update_movie->start();
	}

	void MainWindow::hideUpdate()
	{
		update_movie->setPaused(true);
		update_label->hide();
	}

	////////////////////////////////////////////
	// LockButton
	////////////////////////////////////////////

	LockButton::LockButton(QWidget* parent)
		: QToolButton(parent)
		, locked_pixmap_{ QPixmap(tr(":/bgn/icon_lock_on.png")) }
		, unlocked_pixmap_{ QPixmap(tr(":/bgn/icon_lock_off.png")) }
	{
		setIcon(QIcon{ unlocked_pixmap_ });
		setObjectName(tr("bgn-lock-button"));
	}

	bool LockButton::isLocked() const
	{
		return locked_;
	}

	void LockButton::setLocked(bool v)
	{
		if (locked_ == v)
			return;

		locked_ = v;
		setIcon(QIcon(locked_ ? locked_pixmap_ : unlocked_pixmap_));
		emit toggled(locked_);
	}

	void LockButton::mousePressEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton)
			setLocked(!locked_);
	}
} // namespace GeometryNodes