#include "PLDLiveInstaller.h"
#include "DiskListWidget.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QRegExp>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include <KActionMenu>
#include <KCMultiDialog>
#include <KComboBox>
#include <KDebug>
#include <KLocale>
#include <KPushButton>
#include <KServiceTypeTrader>
#include <KStandardDirs>
#include <KTitleWidget>
#include <KVBox>
#include <kworkspace/kworkspace.h>
#include <kdesu/kdesu_export.h>
#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/StorageDrive>
#include <Solid/StorageVolume>
#include <Solid/StorageAccess>
#include <Solid/Block>

#define SOURCE "/"
#define DESTINATION "/mnt/pldlivechroot/"
#define LIVEUSER "plduser"

PLDLiveInstaller::PLDLiveInstaller( QWidget *parent )
  : KAssistantDialog( parent )
{
    setPlainCaption(i18n("PLD Live Installer"));
    setFixedSize(640,480);
    showButton(KDialog::Help, false);
    showButton(KDialog::Cancel, false);
    showButton(KDialog::User1, false);
   
    // Start
    startPage = addPage(startWidget(), i18n(""));
    
    // Partition
    partitionDialog = kcmDialog("kcm_partitionmanager", "kcm_partitionmanager");
    partitionPage = addPage(kcmLayout(partitionDialog), i18n("Partition your hard drive"));
    if (!partitionDialog)
    {
      setAppropriate(partitionPage, false);
      partition_disk->setCheckable(false);
      partition_disk->setDisabled(true);
      QMessageBox::warning(this, "Can't partition your disk",
			       "Sorry, the kcm_partitionmanager module couldn't be found. You won't "
			       "be able to partition your disk.", QMessageBox::Ok);
    }
    setAppropriate(partitionPage, false);

    // Select destination drive
    selectDiskPage = addPage(selectDiskWidget(), i18n(""));
    setValid(selectDiskPage, false);
    
    //Select partitions
    selectPartitionsPage = addPage(selectPartitionsWidget(), i18n(""));
    setValid(selectPartitionsPage, false);
    
    // Create user account
    createUserPage = addPage(createUserWidget(), i18n(""));
    setValid(createUserPage,false);
    //if (password->text() != password2->text()) setValid(createUserPage, false);
    
    // Copy files to DEST root
    installingPage = addPage(installingWidget(), i18n(""));
    if (pbar->value()!=100) setValid(installingPage, false);
    
    // Finish
    finishPage = addPage(finishWidget(), i18n(""));
    setAppropriate(finishPage, false);
    
    // Fail
    failedPage = addPage(failedWidget(), i18n(""));
    setAppropriate(failedPage, false);

}

PLDLiveInstaller::~PLDLiveInstaller()
{
}

QWidget * PLDLiveInstaller::startWidget()
{
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Install PLD Linux</b></font><br /><br />"
	  "<font size=\"5\"><b>To set up the installation of PLD Linux, click Next</b></font>"
	  "</p></html>"));
	  
    QLabel *text = new QLabel(this);
    text->setText(i18n("<html><p align=\"justify\">"
    "The content of this Live CD will be copied to your hard drive. You will be able to choose a disk, partition it<br />"
    "and create a user. After the installtion is complete you can safely reboot your computer."
    "</p></html>"
    ));
    
    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->addWidget(text);
    
    partition_disk = new QCheckBox("I'd like to partition my disk");
    partition_disk->setCheckState(Qt::Unchecked);
    partition_disk->setChecked(false);    
    connect(partition_disk, SIGNAL( stateChanged(int)), this, SLOT( isChecked(int)));
    
    QVBoxLayout *partLayout = new QVBoxLayout;
    partLayout->addWidget(partition_disk);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(titleWidget);
    layout->addLayout(textLayout);
    layout->addLayout(partLayout);
    layout->setMargin(0);
    
    QWidget *widget = new QWidget;
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    //setMainWidget(widget);
    return widget;
}

void PLDLiveInstaller::isChecked(int state)
{
  if (state == Qt::Checked)
    setAppropriate(partitionPage, true);
  else if (state == Qt::Unchecked)
    setAppropriate(partitionPage, false);
}

void PLDLiveInstaller::isDiskItemSelected()
{
  qDebug() << diskListWidget->currentItem()->text() << "was selected";
  setValid(selectDiskPage, true);
  selectedDisk = diskListWidget->currentItem();
  selectedBlockDev = diskListWidget->devhash->value(selectedDisk->text());
  selHDDisRem = selectedBlockDev.as<Solid::StorageDrive>()->isRemovable();
  qDebug() << "selHDD in isDIsk..." << selHDDisRem;
  defimage->setVisible(false);
  if (selHDDisRem)
  {
    image2->setVisible(true);
    image->setVisible(false);
  }
  else
  {
    image->setVisible(true);
    image2->setVisible(false);
  }
}

void PLDLiveInstaller::isRootPartitionSelected(int pos)
{
  qDebug() << "rootPart Position: " << pos;
  rootPartPos=pos;
  if (pos!=0)
  {
    QString sizeText;
    qlonglong minSize = 3072;
    destPartition=diskListWidget->parthash->value(availablePartitions[pos-1]);
    qlonglong curSize = destPartition.as<Solid::StorageVolume>()->size()/1024/1024; //MB
    QString size = QString::number(curSize);
    if(curSize < minSize)
      sizeText = "Size: " + size + "MB<br /><b>The device is too small for installation.</b><br />You need at least "
		  + QString::number(minSize) + "MB of free space to proceed with installation.";
    else {
      setValid(selectPartitionsPage, true);
      sizeText = "Size: " + size + "MB<br />";
    }
    
    selRoot=destPartition.as<Solid::Block>()->device();
    
    partDescr->setText("Partition details:<br />"
		      "Device: " + selRoot + "<br />"
		      //"File system type: " + destPartition.as<Solid::StorageVolume>()->fsType() + "<br />"
		      + sizeText 
		      //"Mountpoint: " + mountpoint + ""
    );
    //qDebug() << "DestPartition size" << destPartition.as<Solid::StorageVolume>()->size();
   //}
  }
}

void PLDLiveInstaller::isSwapPartitionSelected(int pos)
{
  qDebug() << "swapPart Position: " << pos;
  swapPartPos=pos;
  if (pos!=0)
  {
    destSwap=diskListWidget->parthash->value(availablePartitions[pos-1]);
    selSwap=destSwap.as<Solid::Block>()->device();
  }
}

void PLDLiveInstaller::isFsSelected(int pos)
{
  qDebug() << "FS type: " << fsTypes[pos];
  selFS=fsTypes[pos];
}

void PLDLiveInstaller::reboot()
{
    KWorkSpace::ShutdownConfirm confirm = KWorkSpace::ShutdownConfirmYes;
    KWorkSpace::ShutdownType type = KWorkSpace::ShutdownTypeReboot;
    KWorkSpace::ShutdownMode mode = KWorkSpace::ShutdownModeForceNow;
    KWorkSpace::requestShutDown(confirm, type, mode);
    //QApplication::quit();
    KDialog::delayedDestruct();
}

void PLDLiveInstaller::close()
{
  KDialog::delayedDestruct();
}

QWidget * PLDLiveInstaller::selectDiskWidget()
{   
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Install PLD Linux</b></font><br /><br />"
	  "<font size=\"5\"><b>Select the disk where you want PLD Linux<br /> to install and click Next.</b></font>"
	  "</p></html>"));
	  
    KHBox *buttonBox = new KHBox(this);

    diskListWidget = new DiskListWidget(this);
    connect(diskListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(isDiskItemSelected()));
    diskListWidget->refresh();

    KPushButton *refresh = new KPushButton(KIcon("view-refresh"), i18n("Refresh list"), buttonBox);
    refresh->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    refresh->setToolTip("Refresh device list");
    connect(refresh, SIGNAL(clicked()), diskListWidget, SLOT(refresh()));
    
    defimage = new QLabel;
    defimage->setPixmap(KIcon("media-optical").pixmap(128));
    defimage->setAlignment(Qt::AlignLeft);
    defimage->setVisible(true);
    
    image = new QLabel;
    image->setPixmap(KIcon("drive-harddisk").pixmap(128));
    image->setAlignment(Qt::AlignLeft);
    image->setVisible(false);
    
    image2 = new QLabel;
    image2->setPixmap(KIcon("drive-removable-media-usb").pixmap(128));
    image2->setAlignment(Qt::AlignLeft);
    image2->setVisible(false);
    
    QHBoxLayout *imageLayout = new QHBoxLayout;
    imageLayout->addWidget(defimage);
    imageLayout->addWidget(image);
    imageLayout->addWidget(image2);
	  
    QLabel *text = new QLabel(this);
    text->setText(i18n("<html><p align=\"left\">"
    "The following disks are available:"
    "</p></html>"
    ));
    
    QVBoxLayout *disksLayout = new QVBoxLayout;
    disksLayout->addWidget(text);
    disksLayout->addWidget(diskListWidget);
    disksLayout->addWidget(buttonBox);
    disksLayout->setSpacing(0);
    
    QHBoxLayout *midLayout = new QHBoxLayout;
    midLayout->addLayout(imageLayout);
    midLayout->addLayout(disksLayout);
    midLayout->setMargin(50);
    midLayout->setSpacing(20);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleWidget);
    layout->addLayout(midLayout);
    layout->setMargin(0);
    
    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Maximum);
    return widget;
    
}

QWidget * PLDLiveInstaller::selectPartitionsWidget()
{   
    swapPartPos = 0;
    rootPartPos = 0;
    fsTypes << "ext4" << "ext3" << "ext2" << "xfs";
    selFS=fsTypes[0];
  
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Install PLD Linux</b></font><br /><br />"
	  "<font size=\"5\"><b>Select partitions where you want PLD Linux<br/> to install and click Next.</b></font>"
	  "</p></html>"));
	  
    partImage = new QLabel;
    partImage->setPixmap(KIcon("drive-harddisk").pixmap(128));
    partImage->setAlignment(Qt::AlignLeft);
    partImage->setVisible(false);
    
    partImage2 = new QLabel;
    partImage2->setPixmap(KIcon("drive-removable-media-usb").pixmap(128));
    partImage2->setAlignment(Qt::AlignLeft);
    partImage2->setVisible(false);
    
    partDescr = new QLabel;
    partDescr->setVisible(true);
    partDescr->setAlignment(Qt::AlignLeft);
    
    QHBoxLayout *imageLayout = new QHBoxLayout;
    imageLayout->addWidget(partImage);
    imageLayout->addWidget(partImage2);
    
    QVBoxLayout *descrLayout = new QVBoxLayout;
    descrLayout->addWidget(partDescr);
	  
    selectPartitionPageText = new QLabel(this);
    rootPart = new KComboBox(this);
    swapPart = new KComboBox(this);
    connect(rootPart, SIGNAL(activated(int)), this, SLOT(isRootPartitionSelected(int)));
    connect(swapPart, SIGNAL(activated(int)), this, SLOT(isSwapPartitionSelected(int)));
    fs = new KComboBox(this);
    fs->addItems(fsTypes);
    fs->setCurrentItem("ext4");
    connect(fs, SIGNAL(activated(int)), this, SLOT(isFsSelected(int)));
    
    QLabel *rootText = new QLabel();
    rootText->setText("Root (/)");
    
    QLabel *swapText = new QLabel();
    swapText->setText("Swap");
    
    QHBoxLayout *rootLayout = new QHBoxLayout;
    rootLayout->addWidget(rootText);
    rootLayout->addWidget(rootPart);
    rootLayout->addWidget(fs);
    
    QHBoxLayout *swapLayout = new QHBoxLayout;
    swapLayout->addWidget(swapText);
    swapLayout->addWidget(swapPart);
    
    QVBoxLayout *disksLayout = new QVBoxLayout;
    disksLayout->addWidget(selectPartitionPageText);
    disksLayout->addLayout(rootLayout);
    disksLayout->addLayout(swapLayout);
    disksLayout->setSpacing(0);
    
    QHBoxLayout *midLayout = new QHBoxLayout;
    midLayout->addLayout(imageLayout);
    midLayout->addLayout(disksLayout);
    midLayout->setMargin(50);
    midLayout->setSpacing(20);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleWidget);
    layout->addLayout(midLayout);
    layout->addLayout(descrLayout);
    layout->setMargin(0);
    
    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Maximum);
    return widget;
    
}

void PLDLiveInstaller::isUserEntered(QString user)
{
  if(user!=username->text() || username->text().isEmpty()==false)
    setValid(createUserPage,true);
}

QWidget * PLDLiveInstaller::createUserWidget()
{   
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Install PLD Linux</b></font><br /><br />"
	  "<font size=\"5\"><b>Fill-in the forms below and click Next to continue.</b></font>"
	  "</p></html>"));
	  
    QLabel *image = new QLabel;
    image->setPixmap(KIcon("list-add-user").pixmap(128));
    image->setAlignment(Qt::AlignLeft);
    
    QHBoxLayout *imageLayout = new QHBoxLayout;
    imageLayout->addWidget(image);
    
    username = new QLineEdit;
    username->setText("Username");
    username->setEchoMode(QLineEdit::Normal);
    connect(username, SIGNAL(textEdited(QString)), this, SLOT(isUserEntered(QString)));
    
    password = new QLineEdit;
    password->setText("Password");
    password->setEchoMode(QLineEdit::Password);
    
    password2 = new QLineEdit;
    password2->setText("Re-type password");
    password2->setEchoMode(QLineEdit::Password);
    
    hostname = new QLineEdit;
    hostname->setText("pldmachine");
    hostname->setEchoMode(QLineEdit::Normal);
	  
    QLabel *userText = new QLabel(this);
    userText->setText(i18n("<html>"
    "Username: "
    "</html>"
    ));
    
    QLabel *passwdText = new QLabel(this);
    passwdText->setText(i18n("<html>"
    "Password: "
    "</html>"
    ));
    
    QLabel *passwdText2 = new QLabel(this);
    passwdText2->setText(i18n("<html>"
    "Re-type password: "
    "</html>"
    ));
    
    QLabel *hostnameDesc = new QLabel(this);
    hostnameDesc->setText(i18n("<html>"
      "Set the name of your computer<br/>(if left empty \"pldmachine\" is taken, default \"pldmachine\")"
      "</html>"
    ));
    
    QLabel *hostnameText = new QLabel(this);
    hostnameText->setText(i18n("<html>"
      "Hostname: "
      "</html>"
    ));
    
    QHBoxLayout *usernameLayout = new QHBoxLayout;
    usernameLayout->addWidget(userText);
    usernameLayout->addWidget(username);
    
    QHBoxLayout *passwordLayout = new QHBoxLayout;
    passwordLayout->addWidget(passwdText);
    passwordLayout->addWidget(password);
   
    QHBoxLayout *passwordLayout2 = new QHBoxLayout;
    passwordLayout2->addWidget(passwdText2);
    passwordLayout2->addWidget(password2);
    
    QVBoxLayout *hostnameDescLayout = new QVBoxLayout;
    hostnameDescLayout->addWidget(hostnameDesc);
    
    QHBoxLayout *hostnameLayout = new QHBoxLayout;
    hostnameLayout->addWidget(hostnameText);
    hostnameLayout->addWidget(hostname);
    
    QVBoxLayout *credLayout = new QVBoxLayout;
    credLayout->addLayout(usernameLayout);
    credLayout->addLayout(passwordLayout);
    credLayout->addLayout(passwordLayout2);
    credLayout->addLayout(hostnameDescLayout);
    credLayout->addLayout(hostnameLayout);
    
    QHBoxLayout *midLayout = new QHBoxLayout;
    midLayout->addLayout(imageLayout);
    midLayout->addLayout(credLayout);
    midLayout->setMargin(50);
    midLayout->setSpacing(20);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleWidget);
    layout->addLayout(midLayout);
    layout->setMargin(0);
    
    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    return widget;
}

QWidget * PLDLiveInstaller::installingWidget()
{   
    pbarVal = 0;
  
    KHBox *buttonBox = new KHBox(this);

    install = new KPushButton(KIcon("continue-data-project"), i18n("Install"), buttonBox);
    install->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    install->setToolTip("Start installation process now");
    connect(install, SIGNAL(clicked()), this, SLOT(installation()));
  
  
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Installing</b></font>"
	  "</p></html>"));
	  
    installationHeader = new QLabel(this);
    installationHeader->setText(i18n("<html><p align=\"center\">"
    "Click Install to start installation"
    "</p></html>"
    ));
    
    installationText = new QLabel(this);
    
    pbar = new QProgressBar(this);
    pbar->setRange(0,3);
    pbar->setFormat("%v out of %m (%p%)");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleWidget);
    layout->addWidget(installationHeader);
    layout->addWidget(pbar);
    layout->addWidget(installationText);
    layout->addWidget(buttonBox);
    layout->setMargin(0);
    
    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    return widget;
}

int PLDLiveInstaller::isMounted(Solid::Device partition)
{
  Solid::StorageAccess *storage = partition.as<Solid::StorageAccess>();
  if(storage)
  {
    if(storage->filePath().isEmpty())
      return 0; // false
    else
      return 1; // true
  } else
    return 2; // other, maybe it's a swap space?
}

int PLDLiveInstaller::mountDev(Solid::Device partition)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,-1);
  installationText->setText("<html><p align=\"left\">"
  "Mounting the device"
  "</p></html>");
  qDebug() << "Mounting";
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,-1);
  QStringList args;
  QDir destDir;
  destDir.setPath(DESTINATION);
  if (!destDir.exists())
    destDir.mkdir(destDir.absolutePath());
  args.append(partition.as<Solid::Block>()->device());
  args.append(destDir.absolutePath());
  qDebug() << "dest dir" << destDir.absolutePath();
  QProcess * proc = new QProcess(this);
  proc->start("mount", args);
  proc->waitForFinished(-1);
  updateProgressBar();
  return proc->exitCode();
}

int PLDLiveInstaller::umountDev(Solid::Device partition)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,-1);
  installationText->setText("<html><p align=\"left\">"
  "Unmounting the device"
  "</p></html>");
  qDebug() << "Umounting";
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,-1);
  QStringList args;
  args.append(partition.as<Solid::Block>()->device());
  QProcess * proc = new QProcess(this);
  proc->start("umount", args);
  proc->waitForFinished(-1);
  updateProgressBar();
  return proc->exitCode();
}

void PLDLiveInstaller::makeFS(Solid::Device partition)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,-1);
  installationText->setText("<html><p align=\"left\">"
  "Creating " + selFS + " file system"
  "</p></html>");
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,-1);
  QStringList args;
  args.append("-t");
  args.append(selFS);
  args.append(partition.as<Solid::Block>()->device());
  QProcess * proc = new QProcess(this);
  proc->start("mkfs", args);
  /*while(proc->waitForReadyRead())
  {
    qDebug() << proc->readAllStandardOutput();
  }*/
  proc->waitForFinished(-1);
  updateProgressBar();
  if(proc->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::makeSwap(Solid::Device partition)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,-1);
  installationText->setText("<html><p align=\"left\">"
  "Creating swap space"
  "</p></html>");
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  QStringList args;
  args.append(partition.as<Solid::Block>()->device());
  qDebug() << args;
  QProcess * proc = new QProcess(this);
  proc->start("mkswap", args);
  /*/while(proc->waitForReadyRead())
  {
    qDebug() << proc->readAllStandardOutput();
  }*/
  proc->waitForFinished(-1);
  updateProgressBar();
  if(proc->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::getData(QString src, QString dest)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents,-1);
  installationText->setText("<html><p align=\"left\">"
  "Getting file list... this may take some time."
  "</p></html>");
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  QByteArray output;
  QStringList args;
  args.append("-arx");
  args.append("--stats");
  args.append("--dry-run");
  args.append(src); // from
  args.append(dest); // to
  QProcess * proc = new QProcess(this);
  proc->start("rsync", args);
  while(proc->waitForReadyRead())
    output+=proc->readAll();
  QStringList status = QString::fromLatin1(output).split("\n");
  foreach (QString s, status)
  {
    if(s.contains(QRegExp("^Number of files transferred.*")))
    {
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      s.remove(QRegExp("Number of files transferred: "));
      qDebug() << "files from getData: " << s.toInt();
      pbar->setRange(0,s.toInt());
      pbarVal=1;
    }
  }
  proc->waitForFinished(-1);
  updateProgressBar();
  qDebug() << "getData finished";
}

void PLDLiveInstaller::copyData(QString src, QString dest)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  qDebug() << "Starting copying";
  installationText->setText("<html><p align=\"left\">"
  "Copying data to disk. Grab some coffee and relax."
  "</p></html>");
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  QStringList args,status;
  args.append("-arx");
  args.append("--progress");
  args.append(src); // from
  args.append(dest); // to
  QProcess * proc = new QProcess(this);
  proc->start("rsync", args);
  while(proc->waitForReadyRead())
  {
    status = QString::fromLatin1(proc->readAll()).split("\n");
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    foreach (QString s, status)
    {
      if(s.contains(QRegExp(".*xfer#.*")))
      {
	s.remove(QRegExp(".*xfer#"));
	s.remove(QRegExp(",.*"));
	qDebug() << s;
	pbar->setValue(s.toInt());
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
      }
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
  }
  proc->waitForFinished(-1);
  qDebug() << "Copying finished";
  pbar->setRange(0,11);
  pbarVal=0;
  if (proc->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::move2dest(QString from, QString to)
{
  QFile orig(from), dest(to);
  if (dest.exists())
    dest.remove();
  if (orig.exists())
    orig.rename(to);
}

void PLDLiveInstaller::renameConfigFiles()
{
  qDebug() << "Renaming files";
  QStringList files;
  files << DESTINATION"etc/pld-release.dest" << DESTINATION"etc/X11/kdm/kdmrc.dest" << DESTINATION"etc/fstab.dest" 
	<< DESTINATION"etc/sysconfig/network.dest" << DESTINATION"etc/issue.dest"
	<< DESTINATION"etc/rc.d/rc.local.dest";
  foreach (QString file, files)
  {
    QString oldfile = file;
    qDebug() << "Renaming " << oldfile << " to " << file.remove(QRegExp(".dest$"));
    move2dest(oldfile, file.remove(QRegExp(".dest$")));
  }
}

void PLDLiveInstaller::createHostname()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, -1);
  installationText->setText("<html><p align=\"left\">"
  "Creating hostname entry."
  "</p></html>");
  QString hostnameEntry,networkPath;
  networkPath = DESTINATION"etc/sysconfig/network";
  QFile network(networkPath);
  hostnameEntry = "HOSTNAME=" + selHostname;
  qDebug() << "hostname entry:" << hostnameEntry;
  if (!network.open(QIODevice::Append | QIODevice::Text))
    throw 1;
  QTextStream out(&network);
  out << hostnameEntry;
  network.close();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
}

void PLDLiveInstaller::createFstabEntries()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, -1);
  installationText->setText("<html><p align=\"left\">"
  "Creating fstab entries."
  "</p></html>");
  QString rootEntry,swapEntry,fstabPath;
  fstabPath = DESTINATION"etc/fstab";
  qDebug() << "fstabPath: " << fstabPath;
  QFile fstab(fstabPath);
  rootEntry = selRoot + "\t/\t" + selFS + "\tdefaults\t0\t0\n";
  qDebug() << "fstab root entry:" << rootEntry;
  if (!fstab.open(QIODevice::Append | QIODevice::Text))
    throw 1;
  QTextStream out(&fstab);
  out << rootEntry;
  if(swapPartPos!=0)
  {
    swapEntry = selSwap + "\tswap\tswap\tdefaults\t0\t0\n";
    out << swapEntry;
  }
  fstab.close();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
}

void PLDLiveInstaller::createUser()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Creating user " + selUser + " and it's home directory."
  "</p></html>");
  qDebug() << "Creating user: " << selUser;
  QStringList args;
  args.append(DESTINATION);
  args.append("useradd");
  args.append("-o");
  args.append("-u 1000");
  args.append("-p");
  args.append(genPasswd());
  args.append("-m");
  args.append(selUser);
  QProcess * useradd = new QProcess(this);
  useradd->start("chroot", args);
  useradd->waitForFinished();
  updateProgressBar();
  if (useradd->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::copySettings(QString file)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, -1);
  installationText->setText("<html><p align=\"left\">"
  "Copying user settings."
  "</p></html>");
  QString fromHome, toHome;
  fromHome = SOURCE"home/users/"LIVEUSER"/" + file;
  toHome = DESTINATION"home/users/" + selUser + "/";
  qDebug() << "Copying user settings from:" << fromHome;
  QStringList args;
  args.append("-r");
  args.append("-a");
  args.append(fromHome);
  args.append(toHome);
  QProcess * proc = new QProcess(this);
  proc->start("cp", args);
  proc->waitForFinished();
  updateProgressBar();
  if (proc->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::delUtmpx()
{
  QString utmpxPath;
  utmpxPath = DESTINATION"var/run/utmpx";
  QFile utmpx(utmpxPath);
  if(utmpx.exists())
    utmpx.remove();
}

void PLDLiveInstaller::deleteLiveUser()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Deleting user " LIVEUSER " and it's home directory from destination root."
  "</p></html>");
  qDebug() << "Deleting user: " << LIVEUSER;
  delUtmpx();
  QStringList args;
  args.append(DESTINATION);
  args.append("userdel");
  args.append("-r");
  args.append(LIVEUSER);
  QProcess * userdel = new QProcess(this);
  userdel->start("chroot", args);
  userdel->waitForFinished();
  updateProgressBar();
  if (userdel->exitCode()!=0)
    throw 1;
}

char * PLDLiveInstaller::genPasswd()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, -1);
  char * salt = "$2a$08$";
  char * epasswd = crypt(selPasswd.toLatin1(),salt);
  qDebug() << "Generated passwd: " << epasswd;
  return epasswd;
}

void PLDLiveInstaller::mountProcSysDev()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Binding procfs, sysfs and /dev to destination root."
  "</p></html>");
  qDebug() << "MountProcSysDev";
  QStringList args,sysargs,devargs;
  args.append("-o");
  args.append("bind");
  args.append("/proc");
  args.append(DESTINATION"proc");
  QProcess * proc = new QProcess(this);
  proc->start("mount", args);
  proc->waitForFinished();
  if (proc->exitCode()!=0)
    throw 1;
  QProcess * sys = new QProcess(this);
  sysargs.append("-o");
  sysargs.append("bind");
  sysargs.append("/sys");
  sysargs.append(DESTINATION"sys");
  sys->start("mount", sysargs);
  sys->waitForFinished();
  if (sys->exitCode()!=0)
    throw 1;
  QProcess * dev = new QProcess(this);
  devargs.append("-o");
  devargs.append("bind");
  devargs.append("/dev");
  devargs.append(DESTINATION"dev");
  dev->start("mount", devargs);
  dev->waitForFinished();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  if (dev->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::umountProcSysDev()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Umounting /proc, /sys and /dev."
  "</p></html>");
  qDebug() << "UmountProcSysDev";
  QStringList args,sysargs,devargs;
  args.append(DESTINATION"proc");
  QProcess * proc = new QProcess(this);
  proc->start("umount", args);
  proc->waitForFinished();
  if (proc->exitCode()!=0)
    throw 1;
  QProcess * sys = new QProcess(this);
  sysargs.append(DESTINATION"sys");
  sys->start("umount", sysargs);
  sys->waitForFinished();
  if (sys->exitCode()!=0)
    throw 1;
  QProcess * dev = new QProcess(this);
  devargs.append("-f");
  devargs.append(DESTINATION"dev");
  dev->start("umount", devargs);
  dev->waitForFinished();
  while (dev->exitCode()!=0)
  {
    dev->start("umount", devargs);
    dev->waitForFinished();
  }
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  if (dev->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::geninitrd()
{
  QByteArray output;
  QString kernelver,initrdpath;
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Generating initrd."
  "</p></html>");
  qDebug() << "Generating initrd";
  QStringList args,unameargs;
  QProcess * kernel = new QProcess(this);
  unameargs.append("-r");
  kernel->start("uname", unameargs);
  while(kernel->waitForReadyRead())
    output=kernel->readAllStandardOutput();
  QStringList status = QString::fromLatin1(output).split("\n");
  foreach (QString s, status)
  {
    if(s.contains(QRegExp(".*2.6.*"))) {
      qDebug() << s;
      kernelver = s;
    }
  }
  kernel->waitForFinished();
  initrdpath = "/boot/initrd-" + kernelver + ".gz";
  qDebug() << "Found kernel" << kernelver << "Path" << initrdpath;
  QProcess * geninitrd = new QProcess(this);
  args.append(DESTINATION);
  args.append("geninitrd");
  args.append("-f");
  args.append("--with");
  args.append(selFS);
  args.append(initrdpath);
  args.append(kernelver);
  geninitrd->start("chroot", args);
  geninitrd->waitForFinished();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  if (geninitrd->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::makeGrubConfig()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Creating grub configuration."
  "</p></html>");
  qDebug() << "Creating grub config";
  QStringList args;
  args.append(DESTINATION);
  args.append("grub-mkconfig");
  args.append("-o");
  args.append("/boot/grub/grub.cfg");
  QProcess * grubmkconf = new QProcess(this);
  grubmkconf->start("chroot", args);
  grubmkconf->waitForFinished();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  if (grubmkconf->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::installGrub()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Installing grub."
  "</p></html>");
  qDebug() << "Installing grub on" << selectedBlockDev.as<Solid::Block>()->device();
  QStringList args;
  args.append(DESTINATION);
  args.append("grub-install");
  args.append("--recheck");
  args.append("--force");
  args.append("--no-floppy");
  args.append(selectedBlockDev.as<Solid::Block>()->device());
  QProcess * grubinstall = new QProcess(this);
  grubinstall->start("chroot", args);
  grubinstall->waitForStarted();
  //while(grubinstall->waitForReadyRead())
    //qDebug() << grubinstall->readAll();
  grubinstall->waitForFinished();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  if (grubinstall->exitCode()!=0)
    throw 1;
}

void PLDLiveInstaller::updateProgressBar()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, -1);
  pbar->setValue(++pbarVal);
}

void PLDLiveInstaller::installation()
{
    bool cont = true;
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    KAssistantDialog::setValid(installingPage,false);
    install->setDisabled(true);
    KAssistantDialog::setButtons(None);
    KAssistantDialog::showButtonSeparator(false);
    setAppropriate(failedPage,true);
    installationHeader->setText("<html><p align=\"left\">"
      "Installing PLD Linux on the disk " + selectedDisk->text() + ""
      "</p></html>"
    );
    qDebug() << "IsMounted:" << isMounted(destPartition);
    if(isMounted(destPartition))
      umountDev(destPartition);
    try {
      if(cont)
	makeFS(destPartition);
    }
    catch (int err)
    {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Creating file system failed.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);     
    }
    try
    {
      if(cont) {
	qDebug() << "trying to create swap";
	if(swapPartPos!=0)
	  makeSwap(destSwap);
      }
    }
    catch (int err)
    {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Creating swap space failed.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);   
    }
    mountDev(destPartition);
    getData(SOURCE,DESTINATION);
    try {
      if(cont)
	copyData(SOURCE,DESTINATION);
    }
    catch (int err)
    {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Copying data to destination root failed.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try { 
      if(cont)
	renameConfigFiles();
	createFstabEntries();
    }
    catch (int err)
    {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Creating fstab entries failed.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try { 
      if(cont)
	createHostname();
    }
    catch (int err)
    {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Setting hostname for your machine has failed.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try { 
      if(cont)
	createUser();
    }
    catch (int err) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Creating user " + selUser + " failed.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try { 
      if(cont) {
	copySettings(".kde");
	copySettings(".config");
      }
    }
    catch (int err) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Failed to copy user settings.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try {
      if(cont)
	deleteLiveUser();
    }
    catch (int err) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Failed to delete user " LIVEUSER " from destination root.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try {
      if(cont)
	mountProcSysDev();
    }
    catch (int err) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Failed to bind sysfs, procfs and /dev to destination root.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try {
      if(cont)
	geninitrd();
    }
    catch (int err) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Failed to generate initrd.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try {
      if(cont)
	makeGrubConfig();
    }
    catch (int err) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Failed create grub configuration.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try {
      if(cont)
	installGrub();
    }
    catch (int err) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Failed to install grub on " + selectedDisk->text() + ".<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    try {
      if(cont)
	umountProcSysDev();
    }
    catch (int err) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Failed to umount /proc, /sys and /dev from destination root.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);
    }
    umountDev(destPartition);
    setAppropriate(failedPage,false);
    setAppropriate(finishPage,true);
    KAssistantDialog::setValid(installingPage,true);
    KAssistantDialog::next();
    KAssistantDialog::showButtonSeparator(false);
}

QWidget * PLDLiveInstaller::finishWidget()
{ 
    KHBox *buttonBox = new KHBox(this);

    KPushButton *restart = new KPushButton(KIcon("system-reboot"), i18n("Restart"), buttonBox);
    restart->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    restart->setToolTip("Restart system now");
  
    connect(restart, SIGNAL(clicked()), this, SLOT(reboot()));
    
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Install Succeeded</b></font><br /><br />"
	  "<font size=\"5\">PLD Linux was installed on your disk</font>"
	  "</p></html>"));
	  
    QLabel *image = new QLabel;
    image->setPixmap(KIcon("dialog-ok-apply").pixmap(128));
    image->setAlignment(Qt::AlignCenter);
	  
    QLabel *text = new QLabel(this);
    text->setText(i18n("<html><p align=\"center\">"
    "Your computer must restart to complete the installation.<br /><br />"
    "Click Restart to restart your computer now.<br />"
    "</p></html>"
    ));
    
    QVBoxLayout *midLayout = new QVBoxLayout;
    midLayout->addWidget(image);
    midLayout->addWidget(text);
    midLayout->addWidget(buttonBox);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleWidget);
    layout->addLayout(midLayout);
    layout->setMargin(0);
    
    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    return widget;
}

QWidget * PLDLiveInstaller::failedWidget()
{ 
    KHBox *buttonBox = new KHBox(this);

    KPushButton *close = new KPushButton(KIcon("dialog-close"), i18n("Close"), buttonBox);
    close->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    close->setToolTip("Close the installer");
  
    connect(close, SIGNAL(clicked()), this, SLOT(close()));
    
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Install FAILED</b></font><br /><br />"
	  "<font size=\"5\">PLD Linux was NOT installed on your disk</font>"
	  "</p></html>"));
	  
    QLabel *image = new QLabel;
    image->setPixmap(KIcon("dialog-error").pixmap(128));
    image->setAlignment(Qt::AlignCenter);
	  
    failText = new QLabel(this);
    failText->setText(i18n("<html><p align=\"center\">"
    "This shouldn't happen, but nobody's perfect and what the hell do I know.<br /><br />"
    "Click Close to close the installer.<br />"
    "</p></html>"
    ));
    
    QVBoxLayout *midLayout = new QVBoxLayout;
    midLayout->addWidget(image);
    midLayout->addWidget(failText);
    midLayout->addWidget(buttonBox);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleWidget);
    layout->addLayout(midLayout);
    layout->setMargin(0);
    
    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    return widget;
}

QWidget * PLDLiveInstaller::kcmLayout(KCMultiDialog* dialog)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(dialog);
    layout->setMargin(0);

    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    return widget;
}

KCMultiDialog* PLDLiveInstaller::kcmDialog(const QString lib, const QString name, bool debug)
{
  KService::List serviceList = KServiceTypeTrader::self()->query("KCModule");
  serviceList.append(KServiceTypeTrader::self()->query("SystemSettingsCategory"));
  
  foreach (const KService::Ptr service, serviceList) {
    if (service->library() == lib && ( name.isEmpty() || service->desktopEntryName() == name))
    {
      if (debug) kDebug() <<service->library() << service->desktopEntryName() << service->name();
      KCMultiDialog *dlg = new KCMultiDialog(this);
      dlg->addModule(KCModuleInfo(service));
      dlg->showButton(KDialog::Apply, false);
      dlg->showButton(KDialog::Ok, false);
      dlg->showButton(KDialog::Cancel, false);
      dlg->showButton(KDialog::Help, false);
      dlg->showButton(KDialog::Default, false);
      dlg->showButtonSeparator(false);
      return dlg;
    } else if (debug) kDebug() <<service->library() << service->desktopEntryName() << service->name();
  }

  return 0;
}

void PLDLiveInstaller::back()
{
    if(currentPage() == selectPartitionsPage)
      partDescr->clear();
    
    KAssistantDialog::back();
}

void PLDLiveInstaller::next()
{ 
      if (currentPage() == partitionPage)
	partitionDialog->button(KDialog::Apply)->click();
      
      if (currentPage() == selectDiskPage) {
	qDebug() << "Selected disk:" << selectedDisk->toolTip();
	if(availablePartitions.count()>0)
	  availablePartitions.clear();
	selectedBlockDev = diskListWidget->devhash->value(selectedDisk->text());
	if (selectedBlockDev.udi() != NULL) {
	QString dev = selectedBlockDev.as<Solid::Block>()->device();
	foreach (Solid::Device partDev, Solid::Device::listFromType(Solid::DeviceInterface::Block, selectedBlockDev.udi()))
	{
	  QString part = partDev.as<Solid::Block>()->device();
	  availablePartitions.append(part);
	  qDebug() << dev << "has partition: " << part;
	}
	selHDDisRem = selectedBlockDev.as<Solid::StorageDrive>()->isRemovable();
	if (selHDDisRem)
	{
	  partImage2->setVisible(true);
	  partImage->setVisible(false);
	}
	else
	{
	  partImage->setVisible(true);
	  partImage2->setVisible(false);
	}
	qDebug() << "selHDDisRem" << selHDDisRem;
	availablePartitions.removeDuplicates();
	qDebug() << "Partitions: " << availablePartitions;
	if(rootPart->count()>0)
	  rootPart->clear();
	if(swapPart->count()>0)
	  swapPart->clear();
	rootPart->addItem("Select root partition");
	swapPart->addItem("Select swap partition");
	rootPart->addItems(availablePartitions);
	swapPart->addItems(availablePartitions);
	qDebug() << "Rootpart count: " << rootPart->count();
	qDebug() << "swappart count: " << swapPart->count();
	selectPartitionPageText->setText("<html><p align=\"center\">"
        "You've selected disk: <b>" + dev + "</b><br />"
        "Now select where you want to install the system and swap</p></html>");
	}
      }
      
      if (currentPage() == selectPartitionsPage)
      {
	if((rootPartPos==swapPartPos) || (rootPartPos==0))
	{
	  QMessageBox::critical(this, "Wrong selection",
			       "Sorry, you've selected the same partition twice.<br /><br />"
			       "<b>Please select a different root/swap partition.</b>", 
				QMessageBox::Ok);
	  KAssistantDialog::back();
	}
	if(swapPartPos==0)
	{
	  QMessageBox::warning(this, "No swap partition selected",
			       "No swap partition was selected. This isn't critical.<br /><br />"
			       "<b>Your installation will procceed without setting up swap.</b>", 
				QMessageBox::Ok);
	}
      }
	
      if (currentPage() == createUserPage)
      {
	qDebug() << "Username is" << username->text();
	qDebug() << "Password is" << password->text();
	if ((username->text()=="Username") || username->text().isEmpty())
	{
	  QMessageBox::critical(this, "Wrong or empty username",
				"The username didn't change or is empty.<br /><br />"
				"Please correct the username.",
				QMessageBox::Ok);
	  //KAssistantDialog::back();
	}
	if (password->text()!=password2->text())
	{
	  QMessageBox::critical(this, "Passwords do not match",
				"The passwords are different.<br /><br />"
				"Please set the same password twice.",
				QMessageBox::Ok);
	  KAssistantDialog::back();
	}
	if(hostname->text().isEmpty())
	  selHostname="pldmachine";
	else
	  selHostname=hostname->text();
	selUser=username->text();
	selPasswd=password->text();
      }

      KAssistantDialog::next();
}

#include "PLDLiveInstaller.moc"
