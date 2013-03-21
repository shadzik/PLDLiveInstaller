#include "PLDLiveInstaller.h"
#include "DiskListWidget.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPalette>
#include <QProcess>
#include <QProgressBar>
#include <QRegExp>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include <KActionMenu>
#include <KComboBox>
#include <KDebug>
#include <KLocale>
#include <KPushButton>
#include <KRun>
#include <KService>
#include <KServiceTypeTrader>
#include <KStandardDirs>
#include <KTitleWidget>
#include <KVBox>
#include <kworkspace/kworkspace.h>
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
    setFixedSize(780,550);
    showButton(KDialog::Help, false);
    showButton(KDialog::Cancel, false);
    showButton(KDialog::User1, false);
   
    // Start
    startPage = addPage(startWidget(), i18n(""));
    
    // Partition
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

void PLDLiveInstaller::startPM()
{
  KRun::runCommand("partitionmanager", this);
}

QWidget * PLDLiveInstaller::startWidget()
{
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Install PLD Linux</b></font><br /><br />"
	  "<font size=\"5\"><b>To set up the installation of PLD Linux, click Next</b></font>"
	  "</p></html>"));
	  
    QLabel *text = new QLabel(this);
    text->setText(i18n("<html><p align=\"center\">"
    "The content of this Live CD will be copied to your hard drive. You will be able to choose a disk or partition<br />"
    "and create a user. After the installtion is complete you can safely reboot your computer."
    "</p>"
    "</html>"
    ));
    text->setAlignment(Qt::AlignCenter);
    text->setWordWrap(true);
    
    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->addWidget(text);
    
    QVBoxLayout *buttonBoxLayout = new QVBoxLayout;
    buttonBoxLayout->setAlignment(Qt::AlignCenter);
   
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(titleWidget);
    layout->addLayout(textLayout);
    layout->addLayout(buttonBoxLayout);
    layout->setMargin(30);
    
    QWidget *widget = new QWidget;
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    //setMainWidget(widget);
    return widget;
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
    {
      setValid(selectPartitionsPage, false);
      sizeText = "Size: " + size + "MB";
      devSizeInfo->setText("<html>"
      "<strong>The device is too small for installation.</strong><br/>"
      "You need at least " + QString::number(minSize) + "MB of free space."
      "</html>");
      devSizeInfoIcon->setVisible(true);
      devSizeInfo->setVisible(true);
      isPartBigEnough = false;
    }
    else {
      devSizeInfoIcon->setVisible(false);
      devSizeInfo->setVisible(false);
      setValid(selectPartitionsPage, true);
      isPartBigEnough = true;
      sizeText = "Size: " + size + "MB<br />";
    }
    
    selRoot=destPartition.as<Solid::Block>()->device();
    
    partDescr->setText("Partition details:<br />"
		      "Device: " + selRoot + "<br />"
		      //"File system type: " + destPartition.as<Solid::StorageVolume>()->fsType() + "<br />"
		      + sizeText 
		      //"Mountpoint: " + mountpoint + ""
    );
    
    if(swapPartPos==rootPartPos)
    {
      samePartTwiceIcon->setVisible(true);
      samePartTwice->setVisible(true);
      setValid(selectPartitionsPage, false);
    }
    else
    {
      samePartTwiceIcon->setVisible(false);
      samePartTwice->setVisible(false);
      if(isPartBigEnough)
	setValid(selectPartitionsPage, true);
    }
  }
  else
  {
    samePartTwiceIcon->setVisible(false);
    samePartTwice->setVisible(false);
    devSizeInfoIcon->setVisible(false);
    devSizeInfo->setVisible(false);
    setValid(selectPartitionsPage, false);
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
    if(swapPartPos==rootPartPos)
    {
      samePartTwiceIcon->setVisible(true);
      samePartTwice->setVisible(true);
      setValid(selectPartitionsPage, false);
    }
    else
    {
      samePartTwiceIcon->setVisible(false);
      samePartTwice->setVisible(false);
    }
  }
  else
  {
    samePartTwiceIcon->setVisible(false);
    samePartTwice->setVisible(false);
  }
}

void PLDLiveInstaller::isFsSelected(int pos)
{
  selFS=fsTypes[pos];
  qDebug() << "FS type: " << selFS;
}

void PLDLiveInstaller::reboot()
{
    //KWorkSpace::ShutdownConfirm confirm = KWorkSpace::ShutdownConfirmYes;
    //KWorkSpace::ShutdownType type = KWorkSpace::ShutdownTypeReboot;
    //KWorkSpace::ShutdownMode mode = KWorkSpace::ShutdownModeForceNow;
    //KWorkSpace::requestShutDown(confirm, type, mode);
    QProcess * proc = new QProcess(this);
    proc->start("reboot");
    proc->waitForFinished();
    if(proc->exitCode() == 0)
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
    diskListWidget->setIconSize(QSize(48,48));
    connect(diskListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(isDiskItemSelected()));
    diskListWidget->refresh();

    KPushButton *refresh = new KPushButton(KIcon("view-refresh"), i18n("Refresh list"), buttonBox);
    refresh->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    refresh->setToolTip("Refresh device list");
    refresh->setIconSize(QSize(22,22));
    refresh->setFixedHeight(33);
    connect(refresh, SIGNAL(clicked()), diskListWidget, SLOT(refresh()));
    
    KPushButton *startPartitionManager= new KPushButton(KIcon("partitionmanager"), i18n("Start Partition Manager"), buttonBox);
    startPartitionManager->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    startPartitionManager->setToolTip("Start Partition Manager");
    startPartitionManager->setIconSize(QSize(22,22));
    connect(startPartitionManager, SIGNAL(clicked()), this, SLOT(startPM()));
    
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
    text->setFixedHeight(40);
    
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

void PLDLiveInstaller::instBootLoaderIsChecked(int state)
{
  qDebug() << "Install boot loader?" << state;
  installBootLoader = state;
  if(!installBootLoader)
    qDebug() << "Boot loader will be installed";
  else
    qDebug() << "No boot loader";
}

void PLDLiveInstaller::shouldBeFormated(int pos)
{
  if(pos == 1)
    doFormat=false;
  else
    doFormat=true;
  
  qDebug() << "Should be formated: " << doFormat;
}

QWidget * PLDLiveInstaller::selectPartitionsWidget()
{   
    swapPartPos = 0;
    rootPartPos = 0;
    installBootLoader = false;
    fsTypes << "ext4" << "ext3" << "ext2" << "xfs";
    formatOpts << "Yes" << "No";
    selFS=fsTypes[0];
    doFormat=true;
  
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
    
    QVBoxLayout *imageLayout = new QVBoxLayout;
    imageLayout->addWidget(partImage);
    imageLayout->addWidget(partImage2);
    imageLayout->addWidget(partDescr);
    
    selectPartitionPageText = new QLabel(this);
    rootPart = new KComboBox(this);
    rootPart->setFixedWidth(200);
    swapPart = new KComboBox(this);
    swapPart->setFixedWidth(200);
    connect(rootPart, SIGNAL(activated(int)), this, SLOT(isRootPartitionSelected(int)));
    connect(swapPart, SIGNAL(activated(int)), this, SLOT(isSwapPartitionSelected(int)));
    fs = new KComboBox(this);
    fs->addItems(fsTypes);
    fs->setCurrentItem("ext4");
    connect(fs, SIGNAL(activated(int)), this, SLOT(isFsSelected(int)));
    
    KComboBox *formatBox = new KComboBox(this);
    formatBox->addItems(formatOpts);
    formatBox->setCurrentIndex(0);
    connect(formatBox, SIGNAL(activated(int)), this, SLOT(shouldBeFormated(int)));
    
    QLabel *rootText = new QLabel;
    rootText->setText("Root (/):");
    rootText->setAlignment(Qt::AlignRight);
    rootText->setFixedWidth(150);
    
    QLabel *formatText = new QLabel;
    formatText->setText("Format rootfs?:");
    formatText->setAlignment(Qt::AlignRight);
    formatText->setFixedWidth(150);
    
    QLabel *swapText = new QLabel;
    swapText->setText("Swap:");
    swapText->setAlignment(Qt::AlignRight);
    swapText->setFixedWidth(150);
    
    devSizeInfo = new QLabel;
    devSizeInfo->setAlignment(Qt::AlignLeft);
    devSizeInfo->setMargin(10);
    devSizeInfo->setVisible(false);
    
    devSizeInfoIcon = new QLabel;
    devSizeInfoIcon->setPixmap(KIcon("dialog-error").pixmap(32));
    devSizeInfoIcon->setAlignment(Qt::AlignRight);
    devSizeInfoIcon->setVisible(false);
    
    samePartTwice = new QLabel;
    samePartTwice->setText("<html>"
    "You've selected the same partition twice!"
    "</html>");
    samePartTwice->setAlignment(Qt::AlignLeft);
    samePartTwice->setMargin(10);
    samePartTwice->setVisible(false);
    
    samePartTwiceIcon = new QLabel;
    samePartTwiceIcon->setPixmap(KIcon("dialog-error").pixmap(32));
    samePartTwiceIcon->setAlignment(Qt::AlignRight);
    samePartTwiceIcon->setVisible(false);
    
    QCheckBox *instBootLoader = new QCheckBox("Don't install GRUB boot loader on the root device.");
    instBootLoader->setCheckState(Qt::Unchecked);
    instBootLoader->setChecked(false);    
    connect(instBootLoader, SIGNAL(stateChanged(int)), this, SLOT(instBootLoaderIsChecked(int)));
    
    QHBoxLayout *instBlLayout = new QHBoxLayout;
    instBlLayout->addWidget(instBootLoader);
    instBlLayout->setMargin(0);
    
    QHBoxLayout *devSizeLayout = new QHBoxLayout;
    devSizeLayout->addWidget(devSizeInfoIcon);
    devSizeLayout->addWidget(devSizeInfo);
    devSizeLayout->setAlignment(Qt::AlignLeft);
    devSizeLayout->setMargin(0);
    
    QHBoxLayout *samePartTwiceLayout = new QHBoxLayout;
    samePartTwiceLayout->addWidget(samePartTwiceIcon);
    samePartTwiceLayout->addWidget(samePartTwice);
    samePartTwiceLayout->setAlignment(Qt::AlignLeft);
    samePartTwiceLayout->setMargin(0);
    
    QHBoxLayout *rootLayout = new QHBoxLayout;
    rootLayout->addWidget(rootText);
    rootLayout->addWidget(rootPart);
    rootLayout->addWidget(fs);
    rootLayout->addStretch();
    
    QHBoxLayout *formatLayout = new QHBoxLayout;
    formatLayout->addWidget(formatText);
    formatLayout->addWidget(formatBox);
    formatLayout->addStretch();
    
    QHBoxLayout *swapLayout = new QHBoxLayout;
    swapLayout->addWidget(swapText);
    swapLayout->addWidget(swapPart);
    swapLayout->addStretch();
    
    QVBoxLayout *disksLayout = new QVBoxLayout;
    disksLayout->addWidget(selectPartitionPageText);
    disksLayout->addLayout(rootLayout);
    disksLayout->addLayout(formatLayout);
    disksLayout->addLayout(swapLayout);
    disksLayout->addLayout(devSizeLayout);
    disksLayout->addLayout(samePartTwiceLayout);
    disksLayout->addStretch();
    disksLayout->setSpacing(20);
    
    QHBoxLayout *midLayout = new QHBoxLayout;
    midLayout->addLayout(imageLayout);
    midLayout->addLayout(disksLayout);
    //midLayout->addStretch();
    midLayout->setMargin(20);
    midLayout->setSpacing(50);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(titleWidget);
    layout->addSpacing(20);
    layout->addLayout(midLayout);
    layout->addLayout(instBlLayout);
    layout->setMargin(0);
    
    QWidget *widget = new QWidget(this);
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Maximum);
    return widget;
    
}

void PLDLiveInstaller::isUserEntered()
{
  if(username->text().isEmpty() || username->text()=="Username")
    userSet=false;
  else
    userSet=true;
  if(userSet && pwMatch)
    setValid(createUserPage,true);
  else
    setValid(createUserPage,false);
}

// Password strength meter, stolen from kdelibs/kdeui/dialogs/knewpassworddialog.cpp

int PLDLiveInstaller::effectivePasswordLength(const QString& password)
{
  enum Category {
        Digit,
        Upper,
        Vowel,
        Consonant,
        Special
  };

  Category previousCategory = Vowel;
  QString vowels("aeiou");
  int count = 0;

  for (int i = 0; i < password.length(); ++i) {
    QChar currentChar = password.at(i);
    if (!password.left(i).contains(currentChar)) {
      Category currentCategory;
      switch (currentChar.category()) {
	case QChar::Letter_Uppercase:
	  currentCategory = Upper;
	  break;
	case QChar::Letter_Lowercase:
	  if (vowels.contains(currentChar)) {
	    currentCategory = Vowel;
	  } else {
	    currentCategory = Consonant;
	  }
	  break;
	case QChar::Number_DecimalDigit:
	  currentCategory = Digit;
	  break;
	default:
	  currentCategory = Special;
	  break;
    }
    switch (currentCategory) {
      case Vowel:
	if (previousCategory != Consonant) {
	  ++count;
	}
      break;
      case Consonant:
	if (previousCategory != Vowel) {
	  ++count;
	}
	break;
      default:
	if (previousCategory != currentCategory) {
	  ++count;
	}
	break;
     }
     previousCategory = currentCategory;
    }
  }
  return count;
}

void PLDLiveInstaller::checkPwStrength()
{
  QPalette pal = pwStrengthMeter->palette();
  int pwstrength = (20 * password->text().length() + 80 * effectivePasswordLength(password->text())) / 10;
  if (pwstrength < 0) {
    pwstrength = 0;
  } else if (pwstrength > 100) {
    pwstrength = 100;
  }
  
  if (pwstrength >= 80)
  {
    pwStrengthMeter->setFormat("Very strong");
    pal.setColor(QPalette::Highlight, QColor("green"));
  }
  else if(pwstrength >= 60)
  {
    pwStrengthMeter->setFormat("Strong");
    pal.setColor(QPalette::Highlight, QColor("#009900"));
  }
  else if (pwstrength >= 40)
  {
    pwStrengthMeter->setFormat("Good");
    pal.setColor(QPalette::Highlight, QColor("#CCCC00"));
  }
  else if (pwstrength >= 20)
  {
    pwStrengthMeter->setFormat("Weak");
    pal.setColor(QPalette::Highlight, QColor("orange"));
  }
  else
  {
    pwStrengthMeter->setFormat("Very Weak!!");
    pal.setColor(QPalette::Highlight, QColor("red"));
  }
  
  pwStrengthMeter->setValue(pwstrength);
  pwStrengthMeter->setPalette(pal);
  
  if(userSet && (password->text()==password2->text()))
    setValid(createUserPage,true);
  else
    setValid(createUserPage,false);
}

void PLDLiveInstaller::checkPwMatch()
{
  QPalette pal = pwStrengthMeter->palette();
  pwStrengthMeter->setValue(100);
  if(password->text()==password2->text())
  {
    pwStrengthMeter->setFormat("Passwords match");
    pal.setColor(QPalette::Highlight, QColor("green"));
    pwMatch=true;
  } else {
    pwStrengthMeter->setFormat("Passwords don't match");
    pal.setColor(QPalette::Highlight, QColor("red"));
    pwMatch=false;
  }
  pwStrengthMeter->setPalette(pal);
  if(userSet && pwMatch)
    setValid(createUserPage,true);
  else
    setValid(createUserPage,false);
}


QWidget * PLDLiveInstaller::createUserWidget()
{   
    // initial value false
    pwMatch = false;
  
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Install PLD Linux</b></font><br /><br />"
	  "<font size=\"5\"><b>Fill-in the forms below and click Next to continue.</b></font>"
	  "</p></html>"));
	  
    QLabel *image = new QLabel;
    image->setPixmap(KIcon("list-add-user").pixmap(128));
    image->setAlignment(Qt::AlignLeft);
    
    QHBoxLayout *imageLayout = new QHBoxLayout;
    imageLayout->addWidget(image);
    
    usercredentials = new QLineEdit;
    usercredentials->setPlaceholderText("Your name");
    usercredentials->setEchoMode(QLineEdit::Normal);
    usercredentials->setAlignment(Qt::AlignLeft);
    usercredentials->setFixedWidth(200);
    usercredentials->setFixedHeight(23);
    
    username = new QLineEdit;
    username->setPlaceholderText("Username");
    username->setEchoMode(QLineEdit::Normal);
    username->setAlignment(Qt::AlignLeft);
    username->setFixedWidth(150);
    username->setFixedHeight(23);
    connect(username, SIGNAL(textChanged(QString)), this, SLOT(isUserEntered()));
    
    password = new QLineEdit;
    password->setPlaceholderText("Password");
    password->setEchoMode(QLineEdit::Password);
    password->setAlignment(Qt::AlignLeft);
    password->setFixedWidth(150);
    password->setFixedHeight(23);
    connect(password, SIGNAL(textChanged(QString)), this, SLOT(checkPwStrength()));
    
    password2 = new QLineEdit;
    password2->setPlaceholderText("Confirm password");
    password2->setEchoMode(QLineEdit::Password);
    password2->setAlignment(Qt::AlignLeft);
    password2->setFixedWidth(150);
    password2->setFixedHeight(23);
    connect(password2, SIGNAL(textChanged(QString)), this, SLOT(checkPwMatch()));
    
    hostname = new QLineEdit;
    hostname->setPlaceholderText("pldmachine");
    hostname->setEchoMode(QLineEdit::Normal);
    hostname->setAlignment(Qt::AlignLeft);
    hostname->setFixedWidth(150);
    hostname->setFixedHeight(23);
    
    pwStrengthMeter = new QProgressBar(this);
    pwStrengthMeter->setFormat("Password strength");
    pwStrengthMeter->setRange(0,100);
    pwStrengthMeter->setValue(0);
    pwStrengthMeter->setAlignment(Qt::AlignLeft);
    pwStrengthMeter->setFixedWidth(150);
    /*pwStrengthMeter->setStyleSheet("QProgressBar {"
      "border: none;"
      "border-radius: 2px;"
      "background: transparent;"
      "text-align: center;"
    "}");
    */
    
    autoLogin = new QRadioButton;
    autoLogin->setText("Log in automatically");
    autoLogin->setChecked(false);
    
    noAutoLogin = new QRadioButton;
    noAutoLogin->setText("Require my password to log in");
    noAutoLogin->setChecked(true);
    
    QLabel *empty = new QLabel;
    empty->setFixedWidth(200);
    empty->setFixedHeight(23);
    
    QLabel *userCredField = new QLabel(this);
    userCredField->setText("<html>"
      "Your name:"
      "</html>"
    );
    userCredField->setAlignment(Qt::AlignRight);
    userCredField->setFixedWidth(200);
    userCredField->setFixedHeight(23);
	  
    QLabel *userText = new QLabel(this);
    userText->setText(i18n("<html>"
    "Username: "
    "</html>"
    ));
    userText->setAlignment(Qt::AlignRight);
    userText->setFixedWidth(200);
    userText->setFixedHeight(23);
    
    QLabel *passwdText = new QLabel(this);
    passwdText->setText(i18n("<html>"
    "Password: "
    "</html>"
    ));
    passwdText->setAlignment(Qt::AlignRight);
    passwdText->setFixedWidth(200);
    passwdText->setFixedHeight(23);
    
    QLabel *passwdText2 = new QLabel(this);
    passwdText2->setText(i18n("<html>"
    "Confirm password: "
    "</html>"
    ));
    passwdText2->setAlignment(Qt::AlignRight);
    passwdText2->setFixedWidth(200);
    passwdText2->setFixedHeight(23);
    
    QLabel *passwdStrength = new QLabel;
    passwdStrength->setAlignment(Qt::AlignRight);
    passwdStrength->setFixedWidth(200);
    passwdStrength->setFixedHeight(23);
    
    QLabel *hostnameText = new QLabel(this);
    hostnameText->setText(i18n("<html>"
      "Hostname: "
      "</html>"
    ));
    hostnameText->setAlignment(Qt::AlignRight);
    hostnameText->setFixedWidth(200);
    hostnameText->setFixedHeight(23);
    
    QHBoxLayout *usercredLayout = new QHBoxLayout;
    usercredLayout->addWidget(userCredField);
    usercredLayout->addWidget(usercredentials);
    usercredLayout->addStretch();
    
    QHBoxLayout *usernameLayout = new QHBoxLayout;
    usernameLayout->addWidget(userText);
    usernameLayout->addWidget(username);
    usernameLayout->addStretch();
    
    QHBoxLayout *passwordLayout = new QHBoxLayout;
    passwordLayout->addWidget(passwdText);
    passwordLayout->addWidget(password);
    passwordLayout->addStretch();
   
    QHBoxLayout *passwordLayout2 = new QHBoxLayout;
    passwordLayout2->addWidget(passwdText2);
    passwordLayout2->addWidget(password2);
    passwordLayout2->addStretch();
    
    QHBoxLayout *pwStrengthLayout = new QHBoxLayout;
    pwStrengthLayout->addWidget(passwdStrength);
    pwStrengthLayout->addWidget(pwStrengthMeter);
    pwStrengthLayout->addStretch();
    
    QHBoxLayout *hostnameLayout = new QHBoxLayout;
    hostnameLayout->addWidget(hostnameText);
    hostnameLayout->addWidget(hostname);
    hostnameLayout->addStretch();
    
    QHBoxLayout *autologLayout = new QHBoxLayout;
    autologLayout->addWidget(empty);
    autologLayout->addWidget(autoLogin);
    autologLayout->addStretch();
    
    QHBoxLayout *noautologLayout = new QHBoxLayout;
    noautologLayout->addWidget(empty);
    noautologLayout->addWidget(noAutoLogin);
    autologLayout->addStretch();
    
    QVBoxLayout *credLayout = new QVBoxLayout;
    credLayout->addLayout(usercredLayout);
    credLayout->addLayout(usernameLayout);
    credLayout->addLayout(passwordLayout);
    credLayout->addLayout(passwordLayout2);
    credLayout->addLayout(pwStrengthLayout);
    credLayout->addLayout(hostnameLayout);
    credLayout->addLayout(autologLayout);
    credLayout->addLayout(noautologLayout);
    
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

void PLDLiveInstaller::substr(QString file, QString string1, QString string2)
{
  QFile readFile(file), writeFile(file);
  QString line;
  if(!readFile.open(QIODevice::ReadOnly | QIODevice::Text))
    return;
    
  QTextStream in(&readFile);
  while(!in.atEnd())
  {
    line += in.readLine() + "\n";
    if(line.contains(string1))
      line.replace(string1, string2);
  }
  readFile.close();
  //qDebug() << line;
  
  if(!writeFile.open(QIODevice::WriteOnly | QIODevice::Text))
    return;
  
  QTextStream out(&writeFile);
  out << line;
  writeFile.close();
}

QWidget * PLDLiveInstaller::installingWidget()
{   
    pbarVal = 0;
  
    QHBoxLayout *buttonBox = new QHBoxLayout;

    install = new QPushButton;
    install->setIcon(KIcon("continue-data-project"));
    install->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    install->setToolTip("Start installation process now");
    install->setFixedHeight(60);
    install->setFixedWidth(200);
    install->setIconSize(QSize(48,48));
    install->setFlat(false);
    install->setText(i18n("Install"));
    connect(install, SIGNAL(clicked()), this, SLOT(installation()));
    
    buttonBox->addWidget(install);
  
    KTitleWidget *titleWidget = new KTitleWidget(this);
    titleWidget->setText(i18n("<html><p align=\"center\"><font size=\"10\"><b>Installing</b></font>"
	  "</p></html>"));
	  
    installationHeader = new QLabel(this);
    installationHeader->setText(i18n("<html><p align=\"center\">"
    "Click <i>Install</i> to start installation"
    "</p></html>"
    ));
    installationHeader->setFixedHeight(60);
    
    installationText = new QLabel(this);
    installationText->setFixedHeight(60);
    
    pbar = new QProgressBar(this);
    pbar->setRange(0,4);
    pbar->setFormat("%v out of %m (%p%)");
    pbar->setFixedWidth(700);
    pbar->setFixedHeight(40);
    pbar->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout *installHeaderBox = new QHBoxLayout;
    installHeaderBox->addWidget(installationHeader);
    
    QHBoxLayout *progressBarBox = new QHBoxLayout;
    progressBarBox->addWidget(pbar);
    
    QHBoxLayout *installTextBox = new QHBoxLayout;
    installTextBox->addWidget(installationText);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(titleWidget);
    layout->addStretch();
    layout->addLayout(installHeaderBox);
    layout->addLayout(progressBarBox);
    layout->addLayout(installTextBox);
    layout->addLayout(buttonBox);
    layout->addStretch();
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

bool PLDLiveInstaller::mountDev(Solid::Device partition)
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
  proc->deleteLater();
  return proc->exitCode() == 0;
}

bool PLDLiveInstaller::umountDev(Solid::Device partition)
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
  proc->deleteLater();
  return proc->exitCode() == 0;
}

bool PLDLiveInstaller::makeFS(Solid::Device partition)
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
  proc->deleteLater();
  return proc->exitCode() == 0;
}

bool PLDLiveInstaller::makeSwap(Solid::Device partition)
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
  proc->deleteLater();
  return proc->exitCode() == 0;
}

void PLDLiveInstaller::getData(QString src, QString dest)
{
  qDebug() << "getData started";
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
  while(proc->waitForReadyRead(-1))
    output+=proc->readAllStandardOutput();
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
  proc->deleteLater();
  qDebug() << "getData finished";
}

bool PLDLiveInstaller::copyData(QString src, QString dest)
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
  while(proc->waitForReadyRead(-1))
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
  proc->deleteLater();
  return proc->exitCode() == 0;
}

void PLDLiveInstaller::move2dest(QString from, QString to)
{
  QFile orig(from), dest(to);
  if (dest.exists())
    dest.remove();
  if (orig.exists())
    orig.rename(to);
}

void PLDLiveInstaller::renameConfigFiles(QStringList files)
{
  qDebug() << "Renaming files";
  foreach (QString file, files)
  {
    QString oldfile = file;
    qDebug() << "Renaming " << oldfile << " to " << file.remove(QRegExp(".dest$"));
    move2dest(oldfile, file.remove(QRegExp(".dest$")));
  }
}

bool PLDLiveInstaller::createHostname()
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
    return false;
  QTextStream out(&network);
  out << hostnameEntry;
  network.close();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  return true;
}

bool PLDLiveInstaller::createFstabEntries()
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
      return false;
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
  return true;
}

bool PLDLiveInstaller::createUser()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Creating user " + selUser + " and it's home directory."
  "</p></html>");
  qDebug() << "Creating user: " << selUser;
  QStringList args,rootargs,wheel;
  args.append(DESTINATION);
  args.append("useradd");
  args.append("-o");
  args.append("-u 1000");
  args.append("-p");
  args.append(genPasswd());
  args.append("-c");
  args.append("'" + selCred + "'");
  args.append("-m");
  args.append(selUser);
  rootargs.append(DESTINATION);
  rootargs.append("usermod");
  rootargs.append("-p");
  rootargs.append(genPasswd());
  rootargs.append("root");
  wheel.append(DESTINATION);
  wheel.append("usermod");
  wheel.append("-G");
  wheel.append("wheel");
  wheel.append(selUser);
  QProcess * useradd = new QProcess(this);
  useradd->start("chroot", args);
  useradd->waitForFinished();
  QProcess * usermod = new QProcess(this);
  usermod->start("chroot", rootargs);
  usermod->waitForFinished();
  QProcess * addToWheel = new QProcess(this);
  addToWheel->start("chroot", wheel);
  addToWheel->waitForFinished();
  updateProgressBar();
  useradd->deleteLater();
  usermod->deleteLater();
  addToWheel->deleteLater();
  return (useradd->exitCode() == 0 && usermod->exitCode() == 0 && addToWheel->exitCode() == 0);
}

bool PLDLiveInstaller::copySettings(QString file)
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
  proc->deleteLater();
  return proc->exitCode() == 0;
}

void PLDLiveInstaller::delUtmpx()
{
  QString utmpxPath;
  utmpxPath = DESTINATION"var/run/utmpx";
  QFile utmpx(utmpxPath);
  if(utmpx.exists())
    utmpx.remove();
}

bool PLDLiveInstaller::deleteLiveUser()
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
  userdel->deleteLater();
  return userdel->exitCode()==0;
}

void PLDLiveInstaller::removeJunk(QString filedir, QString argstorm)
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
    "Cleaning up..."
    "</p></html>"
  );
  qDebug() << "Deleting junk: " << filedir;
  QStringList args;
  args.append(DESTINATION);
  args.append("rm");
  if (!argstorm.isEmpty())
    args.append(argstorm);
  args.append(filedir);
  QProcess * remjunk = new QProcess(this);
  remjunk->start("chroot", args);
  remjunk->waitForFinished(-1);
  updateProgressBar();
  remjunk->deleteLater();
}

char * PLDLiveInstaller::genPasswd()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, -1);
  const char * salt = "$2a$08$";
  char * epasswd = crypt(selPasswd.toLatin1(),salt);
  qDebug() << "Generated passwd: " << epasswd;
  return epasswd;
}

bool PLDLiveInstaller::mountProcSysDev()
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
  proc->deleteLater();
  if (proc->exitCode()!=0)
    return false;
  QProcess * sys = new QProcess(this);
  sysargs.append("-o");
  sysargs.append("bind");
  sysargs.append("/sys");
  sysargs.append(DESTINATION"sys");
  sys->start("mount", sysargs);
  sys->waitForFinished();
  sys->deleteLater();
  if (sys->exitCode()!=0)
    return false;
  QProcess * dev = new QProcess(this);
  devargs.append("-o");
  devargs.append("bind");
  devargs.append("/dev");
  devargs.append(DESTINATION"dev");
  dev->start("mount", devargs);
  dev->waitForFinished();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  dev->deleteLater();
  return dev->exitCode()==0;
}

bool PLDLiveInstaller::umountProcSysDev()
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
  proc->deleteLater();
  if (proc->exitCode()!=0)
    return false;
  QProcess * sys = new QProcess(this);
  sysargs.append(DESTINATION"sys");
  sys->start("umount", sysargs);
  sys->waitForFinished();
  sys->deleteLater();
  if (sys->exitCode()!=0)
    return false;
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
  dev->deleteLater();
  return dev->exitCode()==0;
}

bool PLDLiveInstaller::geninitrd()
{
  QByteArray output, modules;
  QString kernelver,initrdpath, geninitrdfilepath, premodsEntry;
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  installationText->setText("<html><p align=\"left\">"
  "Generating initrd."
  "</p></html>");
  QProcess * getmodules = new QProcess(this);
  getmodules->start("/usr/sbin/getxatakmod.sh");
  while(getmodules->waitForReadyRead())
    modules=getmodules->readAllStandardOutput();
  getmodules->waitForFinished();
  getmodules->deleteLater();
  geninitrdfilepath = DESTINATION"etc/sysconfig/geninitrd";
  QFile geninitrdfile(geninitrdfilepath);
  premodsEntry = "PREMODS=\"" + modules + " " + selFS + "\"";
  if (!geninitrdfile.open(QIODevice::Append | QIODevice::Text))
    return false;
  QTextStream out(&geninitrdfile);
  out << premodsEntry;
  geninitrdfile.close();
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
  kernel->deleteLater();
  initrdpath = "/boot/initrd-" + kernelver + ".gz";
  qDebug() << "Found kernel" << kernelver << "Path" << initrdpath;
  QProcess * geninitrd = new QProcess(this);
  args.append(DESTINATION);
  args.append("geninitrd");
  args.append("-f");
  args.append(initrdpath);
  args.append(kernelver);
  geninitrd->start("chroot", args);
  geninitrd->waitForFinished();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  geninitrd->deleteLater();
  return geninitrd->exitCode()==0;
}

bool PLDLiveInstaller::makeGrubConfig()
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
  grubmkconf->deleteLater();
  return grubmkconf->exitCode()==0;
}

bool PLDLiveInstaller::installGrub()
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
  //grubinstall->waitForStarted();
  //while(grubinstall->waitForReadyRead())
    //qDebug() << grubinstall->readAll();
  grubinstall->waitForFinished();
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  updateProgressBar();
  grubinstall->deleteLater();
  return grubinstall->exitCode()==0;
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
    
    if(doFormat) {
      if(cont && !makeFS(destPartition)) {
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
    }

    if(cont) {
      qDebug() << "trying to create swap";
      if(swapPartPos!=0 && !makeSwap(destSwap)) {
      cont = false;
      failText->setText("<html><p align=\"center\">"
      "Creating swap space failed.<br/><br/>"
      "Click Close to close the installer.<br />"
      "</p></html>"
      );
      KAssistantDialog::setValid(installingPage,true);
      KAssistantDialog::next();
      KAssistantDialog::showButtonSeparator(false);   
      };
    };

    if (cont) {
      mountDev(destPartition);
      getData(SOURCE,DESTINATION);
    }

    if(cont && !copyData(SOURCE,DESTINATION)){
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

    if(cont)
    {
      qDebug() << "renameConfigFiles...";
      QStringList files;
      files << DESTINATION"etc/pld-release.dest" << DESTINATION"etc/fstab.dest" 
	<< DESTINATION"etc/sysconfig/network.dest" << DESTINATION"etc/issue.dest"
	<< DESTINATION"etc/rc.d/rc.local.dest" << DESTINATION"etc/sudoers.dest";
      qDebug() << "QRadioButton" << "noAutoLogin" << noAutoLogin->isChecked() << "autoLogin" << autoLogin->isChecked();
      if(noAutoLogin->isChecked())
	files.append(DESTINATION"etc/X11/kdm/kdmrc.dest");
      if(autoLogin->isChecked())
	substr(DESTINATION"etc/X11/kdm/kdmrc", LIVEUSER, selUser);
      renameConfigFiles(files);
      files.clear();
    }
    
    if(cont && !createFstabEntries())
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
 
      if(cont && !createHostname()) {
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
    
    if(cont && !createUser()) {
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
 
    if(cont && !copySettings(".kde") && !copySettings(".config")) {
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

    if(cont && !deleteLiveUser()) {
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
    
    if(cont) {
     removeJunk("/var/tmp/kdecache-plduser","-rf");
     removeJunk("/usr/sbin/pldliveinstaller", "-f");
     removeJunk("/home/users/" + selUser + "/.kde/share/config/dolphinrc", "-f");
    }

    if(cont && !mountProcSysDev()) {
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

    if(cont && !geninitrd()) {
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

    if(!installBootLoader) // this is true if value is != 0 (false)
    {

      if(cont && !makeGrubConfig()) {
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

      if(cont && !installGrub()) {
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
    } // end if installBootLoader

    if(cont && !umountProcSysDev()) {
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

void PLDLiveInstaller::back()
{
    if(currentPage() == selectPartitionsPage)
    {
      partDescr->clear();
      selRoot.clear();
      selSwap.clear();
    }
    
    KAssistantDialog::back();
}

void PLDLiveInstaller::next()
{ 
      //if (currentPage() == partitionPage)
	//partitionDialog->button(KDialog::Apply)->click();
      
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
	
      if (currentPage() == createUserPage)
      {
	if(hostname->text().isEmpty())
	  selHostname="pldmachine";
	else
	  selHostname=hostname->text();
	if(usercredentials->text().isEmpty())
	  selCred="PLD User";
	else
	  selCred=usercredentials->text();
	selUser=username->text();
	selPasswd=password->text();
	qDebug() << "Username is" << selUser;
	qDebug() << "Password is" << selPasswd;
	qDebug() << "Credentials" << selCred;
	qDebug() << "Hostname" << selHostname; 
      }

      KAssistantDialog::next();
}

#include "PLDLiveInstaller.moc"
