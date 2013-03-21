#ifndef PLDLiveInstaller_H
#define PLDLiveInstaller_H

#include <QCheckBox>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QProgressBar>
#include <KComboBox>
#include <KAssistantDialog>
#include <KDialog>
#include <KAboutData>
#include <QHash>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <QLabel>

#include "DiskListWidget.h"

class KCMultiDialog;

class PLDLiveInstaller : public KAssistantDialog
{

    Q_OBJECT

    KCMultiDialog *partitionDialog;
    KPageWidgetItem *startPage;
    KPageWidgetItem *partitionPage;
    KPageWidgetItem *selectDiskPage;
    KPageWidgetItem *selectPartitionsPage;
    KPageWidgetItem *createUserPage;
    KPageWidgetItem *installingPage;
    KPageWidgetItem *finishPage;
    KPageWidgetItem *failedPage;

    QWidget * kcmLayout(KCMultiDialog* dialog);
    QWidget * startWidget();
    QWidget * selectDiskWidget();
    QWidget * selectPartitionsWidget();
    QWidget * createUserWidget();
    QWidget * installingWidget();
    QWidget * finishWidget();
    QWidget * failedWidget();
    KCMultiDialog* kcmDialog(const QString lib, const QString name = "", bool debug = false);
    QProgressBar *pbar;
    KPushButton *install;
    QCheckBox *partition_disk;
    QLineEdit *username, *password, *password2, *hostname;
    DiskListWidget *diskListWidget;
    QListWidgetItem * selectedDisk;
    Solid::Device selectedBlockDev, destPartition, destSwap;
    QLabel *selectPartitionPageText, *installationText, *installationHeader, *failText;
    QStringList availablePartitions, fsTypes;
    KComboBox * rootPart,* swapPart, * fs;
    int rootPartPos, swapPartPos, pbarVal;
    bool selHDDisRem;
    QLabel *defimage, *image, *image2, *partImage, *partImage2, *partDescr;
    QString selFS, selUser, selPasswd, selHostname, failReason, selRoot, selSwap;
    int isMounted(Solid::Device partition);
    int mountDev(Solid::Device partition), umountDev(Solid::Device partition);
    void copyData(QString src,QString dest), getData(QString src, QString dest);
    void makeFS(Solid::Device partition), makeSwap(Solid::Device partition);
    void updateProgressBar(), createFstabEntries(), createUser(), copySettings(QString file), deleteLiveUser();
    char * genPasswd();
    void makeGrubConfig(), installGrub(), geninitrd(), mountProcSysDev(), umountProcSysDev(), delUtmpx();
    void move2dest(QString from, QString to), renameConfigFiles(), createHostname();
    
public:
    PLDLiveInstaller( QWidget * parent = 0 );
    virtual ~PLDLiveInstaller();
        
public Q_SLOTS:
    virtual void back();
    virtual void next();
    void reboot();
    void close();
    void installation();
    void isChecked(int state);
    void isDiskItemSelected();
    void isRootPartitionSelected(int pos);
    void isSwapPartitionSelected(int pos);
    void isFsSelected(int pos);
    void isUserEntered(QString user);
    
};

#endif // PLDLiveInstaller_H
