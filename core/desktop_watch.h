#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

namespace doc {

// 桌面目录监控：QFileSystemWatcher + QTimer 防抖
// （拷贝大文件过程中会连续触发，合并防抖时长内的变更，到期只发一次信号）
class DesktopWatch : public QObject {
    Q_OBJECT

public:
    explicit DesktopWatch(QObject *parent = nullptr);

    bool start(const QString &dirPath);
    void stop();
    bool isWatching() const;
    QString watchedDir() const;

    void setDebounce(int ms);
    int debounce() const;

signals:
    void changeDetected(const QString &dirPath);

private slots:
    void onDirectoryChanged(const QString &path);

private:
    QFileSystemWatcher m_watcher;
    QTimer m_debounceTimer;
    QString m_dir;
};

} // namespace doc
