#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QObject>
#include <QWidget>
#include <QDialog>

class QQuickWidget;
class MainConfiguration;
class LibFacade;
class QShowEvent;

class ConfigDialog : public QDialog
{
        Q_OBJECT
    public:
        explicit ConfigDialog(QWidget *parent, MainConfiguration *Config, LibFacade *Lib = nullptr);
        ~ConfigDialog();

    void showEvent(QShowEvent *showEvent) override;

    public slots:
        void accept() override;

    private slots:
        void onQmlAccepted();
        void onQmlRejected();

    protected:
        QQuickWidget        *quickWidget;
        MainConfiguration   *MyConfiguration = nullptr;
        LibFacade           *MyLibFacade = nullptr;

};

#endif // CONFIGDIALOG_H
