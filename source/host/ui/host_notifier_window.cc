//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "host/ui/host_notifier_window.h"

#include <QMenu>
#include <QMouseEvent>
#include <QScreen>
#include <QTranslator>

#include "base/logging.h"
#include "build/build_config.h"
#include "host/host_settings.h"

namespace aspia {

namespace {

class SessionTreeItem : public QTreeWidgetItem
{
public:
    SessionTreeItem(const proto::notifier::Session& session)
        : session_(session)
    {
        switch (session_.session_type())
        {
            case proto::SESSION_TYPE_DESKTOP_MANAGE:
                setIcon(0, QIcon(QStringLiteral(":/icon/monitor-keyboard.png")));
                break;

            case proto::SESSION_TYPE_DESKTOP_VIEW:
                setIcon(0, QIcon(QStringLiteral(":/icon/monitor.png")));
                break;

            case proto::SESSION_TYPE_FILE_TRANSFER:
                setIcon(0, QIcon(QStringLiteral(":/icon/folder-stand.png")));
                break;

            default:
                LOG(LS_FATAL) << "Unexpected session type: " << session_.session_type();
                return;
        }

        setText(0, QString("%1 (%2)")
                .arg(QString::fromStdString(session_.username()))
                .arg(QString::fromStdString(session_.remote_address())));
    }

    const proto::notifier::Session& session() const { return session_; }

private:
    proto::notifier::Session session_;
    DISALLOW_COPY_AND_ASSIGN(SessionTreeItem);
};

} // namespace

HostNotifierWindow::HostNotifierWindow(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint)
{
    HostSettings settings;

    QString current_locale = settings.locale();

    if (!locale_loader_.contains(current_locale))
        settings.setLocale(DEFAULT_LOCALE);

    locale_loader_.installTranslators(current_locale);
    ui.setupUi(this);

    ui.label_title->installEventFilter(this);
    ui.label_connections->installEventFilter(this);

    connect(ui.button_show_hide, &QPushButton::pressed,
            this, &HostNotifierWindow::onShowHidePressed);

    connect(ui.button_disconnect_all, &QPushButton::pressed,
            this, &HostNotifierWindow::onDisconnectAllPressed);

    connect(ui.tree, &QTreeWidget::customContextMenuRequested,
            this, &HostNotifierWindow::onContextMenu);

    setAttribute(Qt::WA_TranslucentBackground);

    connect(QApplication::primaryScreen(), &QScreen::availableGeometryChanged,
            this, &HostNotifierWindow::updateWindowPosition);

    updateWindowPosition();
}

void HostNotifierWindow::setChannelId(const QString& channel_id)
{
    channel_id_ = channel_id;
}

void HostNotifierWindow::sessionOpen(const proto::notifier::Session& session)
{
    ui.tree->addTopLevelItem(new SessionTreeItem(session));
    ui.button_disconnect_all->setEnabled(true);
}

void HostNotifierWindow::sessionClose(const proto::notifier::SessionClose& session_close)
{
    for (int i = 0; i < ui.tree->topLevelItemCount(); ++i)
    {
        SessionTreeItem* item = dynamic_cast<SessionTreeItem*>(ui.tree->topLevelItem(i));
        if (item && item->session().uuid() == session_close.uuid())
        {
            delete item;
            break;
        }
    }

    if (!ui.tree->topLevelItemCount())
        quit();
}

bool HostNotifierWindow::eventFilter(QObject* object, QEvent* event)
{
    if (object == ui.label_title || object == ui.label_connections)
    {
        switch (event->type())
        {
            case QEvent::MouseButtonPress:
            {
                QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
                if (mouse_event && mouse_event->button() == Qt::LeftButton)
                {
                    start_pos_ = mouse_event->pos();
                    return true;
                }
            }
            break;

            case QEvent::MouseMove:
            {
                QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
                if (mouse_event && mouse_event->buttons() & Qt::LeftButton && start_pos_.x() >= 0)
                {
                    QPoint diff = mouse_event->pos() - start_pos_;
                    move(pos() + diff);
                    return true;
                }
            }
            break;

            case QEvent::MouseButtonRelease:
            {
                start_pos_ = QPoint(-1, -1);
            }
            break;

            default:
                break;
        }
    }

    return QWidget::eventFilter(object, event);
}

void HostNotifierWindow::showEvent(QShowEvent* event)
{
    if (notifier_.isNull())
    {
        notifier_ = new HostNotifier(this);

        connect(notifier_, &HostNotifier::finished, this, &HostNotifierWindow::quit);
        connect(notifier_, &HostNotifier::sessionOpen, this, &HostNotifierWindow::sessionOpen);
        connect(notifier_, &HostNotifier::sessionClose, this, &HostNotifierWindow::sessionClose);
        connect(this, &HostNotifierWindow::killSession, notifier_, &HostNotifier::killSession);

        if (!notifier_->start(channel_id_))
        {
            quit();
            return;
        }
    }

    QWidget::showEvent(event);
}

void HostNotifierWindow::hideEvent(QHideEvent* event)
{
    show();
}

void HostNotifierWindow::quit()
{
    close();
    QApplication::quit();
}

void HostNotifierWindow::onShowHidePressed()
{
    if (ui.content->isVisible())
        hideNotifier();
    else
        showNotifier();
}

void HostNotifierWindow::onDisconnectAllPressed()
{
    for (int i = 0; i < ui.tree->topLevelItemCount(); ++i)
    {
        SessionTreeItem* item = dynamic_cast<SessionTreeItem*>(ui.tree->topLevelItem(i));
        if (item)
            emit killSession(item->session().uuid());
    }
}

void HostNotifierWindow::onContextMenu(const QPoint& point)
{
    SessionTreeItem* item = dynamic_cast<SessionTreeItem*>(ui.tree->itemAt(point));
    if (!item)
        return;

    QAction disconnect_action(tr("Disconnect"));

    QMenu menu;
    menu.addAction(&disconnect_action);

    if (menu.exec(ui.tree->viewport()->mapToGlobal(point)) == &disconnect_action)
        emit killSession(item->session().uuid());
}

void HostNotifierWindow::updateWindowPosition()
{
    showNotifier();

    QRect screen_rect = QApplication::primaryScreen()->availableGeometry();
    QSize window_size = frameSize();

    move(screen_rect.x() + screen_rect.width() - window_size.width(),
         screen_rect.y() + screen_rect.height() - window_size.height());
}

void HostNotifierWindow::showNotifier()
{
    if (ui.content->isHidden() && ui.title->isHidden())
    {
        ui.content->show();
        ui.title->show();

        move(window_rect_.topLeft());
        setFixedSize(window_rect_.size());

        ui.button_show_hide->setIcon(QIcon(QStringLiteral(":/icon/arrow-left-gray.png")));
    }
}

void HostNotifierWindow::hideNotifier()
{
    QRect screen_rect = QApplication::primaryScreen()->availableGeometry();
    QSize content_size = ui.content->frameSize();
    window_rect_ = frameGeometry();

    ui.content->hide();
    ui.title->hide();

    move(screen_rect.x() + screen_rect.width() - ui.button_show_hide->width(), pos().y());

    setFixedSize(window_rect_.width() - content_size.width(),
                 window_rect_.height());

    ui.button_show_hide->setIcon(QIcon(QStringLiteral(":/icon/arrow-right-gray.png")));
}

} // namespace aspia
