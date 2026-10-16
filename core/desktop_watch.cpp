#include "desktop_watch.h"

#include <QDir>

namespace doc {

DesktopWatch::DesktopWatch(QObject *parent)
    : QObject(parent)
{
    m_debounceTimer.setSingleShot(true);
    connect(&m_debounceTimer, &QTimer::timeout, this, [this]() {
        emit changeDetected(m_dir);
    });
}

bool DesktopWatch::start(const QString &dirPath)
{
    stop();

    if (!QDir(dirPath).exists())
        return false;

    if (!m_watcher.addPath(dirPath))
        return false;

    m_dir = dirPath;
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &DesktopWatch::onDirectoryChanged);
    return true;
}

void DesktopWatch::stop()
{
    if (!m_dir.isEmpty())
        m_watcher.removePath(m_dir);
    m_dir.clear();
    m_debounceTimer.stop();
}

bool DesktopWatch::isWatching() const
{
    return !m_dir.isEmpty();
}

QString DesktopWatch::watchedDir() const
{
    return m_dir;
}

void DesktopWatch::setDebounce(int ms)
{
    m_debounceTimer.setInterval(qMax(0, ms));
}

int DesktopWatch::debounce() const
{
    return m_debounceTimer.interval();
}

void DesktopWatch::onDirectoryChanged(const QString &path)
{
    // 目录被移动/重命名后可能掉出监控列表，尽量补回来
    if (!m_watcher.directories().contains(path) && QDir(path).exists())
        m_watcher.addPath(path);

    m_debounceTimer.start();
}

} // namespace doc
