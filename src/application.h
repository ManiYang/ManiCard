#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class MainWindow;

class Application : public QObject
{
    Q_OBJECT
public:

    explicit Application(QObject *parent = nullptr);

    ~Application();

    //!
    //! Services must already be set up before this method is called.
    //!
    void initialize();

    void loadOnStart();

    void activateMainWindow();

signals:

private:
    MainWindow *mainWindow {nullptr};

    //!
    //! Before this method is called:
    //!   + \c mainWindow must be visible
    //!   + \c mainWindow->canReload() must return true.
    //!
    void reload(std::function<void (bool ok)> callback);

    void onUserToReloadApp();
};

#endif // APPLICATION_H
