#ifndef DiskListWidget_H
#define DiskListWidget_H

#include <QCheckBox>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QProgressBar>
#include <KAssistantDialog>
#include <KDialog>
#include <KAboutData>
#include <QHash>
#include <Solid/Device>

class DiskListWidget;

class DiskListWidget : public QListWidget
{

    Q_OBJECT

    QStringList diskList;
    QStringList diskListAlreadyOnList;

public:
    DiskListWidget( QWidget * parent = 0 );
    QListWidgetItem *listitem;
    QHash<QString,Solid::Device> * devhash, * parthash;
    virtual ~DiskListWidget();
    
public Q_SLOTS:
    void refresh();
};

#endif // DiskListWidget_H
