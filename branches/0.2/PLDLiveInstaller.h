#ifndef PLDLiveInstaller_H
#define PLDLiveInstaller_H

#include <QCheckBox>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QRadioButton>
#include <KComboBox>
#include <KAssistantDialog>
#include <KDialog>
#include <KAboutData>
#include <QHash>
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <QLabel>
#include <QPushButton>
#include <KPushButton>

#include "DiskListWidget.h"

class PLDLiveInstaller : public KAssistantDialog
{

    Q_OBJECT

    KPageWidgetItem *startPage;
    KPageWidgetItem *partitionPage;
    KPageWidgetItem *selectDiskPage;
    KPageWidgetItem *selectPartitionsPage;
    KPageWidgetItem *createUserPage;
    KPageWidgetItem *installingPage;
    KPageWidgetItem *finishPage;
    KPageWidgetItem *failedPage;

    QWidget * startWidget();
    QWidget * selectDiskWidget();
    QWidget * selectPartitionsWidget();
    QWidget * createUserWidget();
    QWidget * installingWidget();
    QWidget * finishWidget();
    QWidget * failedWidget();
    QProgressBar *pbar, *pwStrengthMeter;
    QPushButton *install;
    QLineEdit *username, *password, *password2, *hostname, *usercredentials;
    DiskListWidget *diskListWidget;
    QListWidgetItem * selectedDisk;
    Solid::Device selectedBlockDev, destPartition, destSwap;
    QLabel *selectPartitionPageText, *installationText, *installationHeader, *failText;
    QLabel *devSizeInfo, *devSizeInfoIcon, *samePartTwice, *samePartTwiceIcon;
    QStringList availablePartitions, fsTypes, formatOpts;
    KComboBox * rootPart,* swapPart, * fs;
    QRadioButton *autoLogin, *noAutoLogin;
    int rootPartPos, swapPartPos, pbarVal;
    bool selHDDisRem, isPartBigEnough, installBootLoader, pwMatch, userSet, doFormat;
    QLabel *defimage, *image, *image2, *partImage, *partImage2, *partDescr;
    QString selFS, selUser, selPasswd, selHostname, failReason, selRoot, selSwap, selCred;
    int isMounted(Solid::Device partition);
    bool mountDev(Solid::Device partition), umountDev(Solid::Device partition);
    bool copyData(QString src,QString dest);
    void getData(QString src, QString dest);
    bool makeFS(Solid::Device partition), makeSwap(Solid::Device partition);
    void updateProgressBar();
    char * genPasswd();
    bool makeGrubConfig(), installGrub(), geninitrd(), mountProcSysDev(), umountProcSysDev();
    void move2dest(QString from, QString to), renameConfigFiles(QStringList files), delUtmpx();
    bool createHostname(), createFstabEntries(), createUser(), copySettings(QString file), deleteLiveUser();
    int effectivePasswordLength(const QString& password);
    void removeJunk(QString filedir, QString argstorm);
    void substr(QString file, QString string1, QString string2);
    
public:
    PLDLiveInstaller( QWidget * parent = 0 );
    virtual ~PLDLiveInstaller();
        
public Q_SLOTS:
    virtual void back();
    virtual void next();
    void reboot();
    void close();
    void installation();
    void isDiskItemSelected();
    void isRootPartitionSelected(int pos);
    void isSwapPartitionSelected(int pos);
    void isFsSelected(int pos);
    void shouldBeFormated(int pos);
    void isUserEntered();
    void instBootLoaderIsChecked(int state);
    void checkPwStrength();
    void checkPwMatch();
    void startPM();
};

#endif // PLDLiveInstaller_H
