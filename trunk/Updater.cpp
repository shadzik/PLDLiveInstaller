#include "Updater.h"
#include "PLDLiveInstaller.h"
#include "Version.h"

#include <KTitleWidget>
#include <KLocale>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QLabel>
#include <QMessageBox>
#include <QString>
#include <QTextEdit>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

Updater::Updater(QWidget* parent) : QObject(parent)
{
}

Updater::~Updater()
{
}

bool Updater::shouldWeCheckForUpdate()
{
  bool result = false;
  QFile *lastChecked = new QFile(QDir::homePath() + "/.config/pliLastChecked");
  QDateTime *datetime = new QDateTime;
  if(lastChecked->exists())
  {
    if (!lastChecked->open(QIODevice::ReadWrite))
    {
      qDebug() << "Couldn't open last check file";
      delete lastChecked;
      lastChecked = 0;
      result = true;
    }
    else
    {
      QTextStream in(lastChecked);
      qlonglong line = in.readLine().toLongLong();
      //qDebug() << "line:" << line;
      int difference = datetime->currentDateTime().toTime_t() - line;
      if (difference >= 2592000) // about a month
      {
	lastChecked->close();
	lastChecked->remove();
	if(lastChecked->open(QIODevice::WriteOnly))
	{
	  QTextStream out(lastChecked);
	  out << datetime->currentDateTime().toTime_t() << "\n";
	  lastChecked->close();
	  result = true;
	}
      }
      else
      {
	lastChecked->close();
	result = false;
      }
    }
  }
  else
  {
    if (lastChecked->open(QIODevice::WriteOnly))
    {
      QTextStream out(lastChecked);
      out << datetime->currentDateTime().toTime_t() << "\n";
      lastChecked->close();
      qDebug() << "Wrote last checked";
      result = true;
    }
  }
  return result;
}

void Updater::startInstaller()
{
  PLDLiveInstaller * start = new PLDLiveInstaller;
  start->setCaption("PLD Live Installer");
  start->exec();
}

void Updater::getLatestVersion()
{
  if(shouldWeCheckForUpdate())
  {
    QString fileName = "/tmp/installerUpdate.xml";
    QUrl url("http://pldliveinstaller.pld-linux.org/installerUpdate.xml");
    if (QFile::exists(fileName))
      QFile::remove(fileName);
    tmpfile = new QFile(fileName);
    if (tmpfile->isOpen())
      tmpfile->close();
    if (!tmpfile->open(QIODevice::WriteOnly)) {
      qDebug() << "Couldn't save XML file";
      delete tmpfile;
      tmpfile = 0;
      return;
    }
    
    http = new QHttp;
    http->setHost(url.host(), url.port(80));
    httpRequestAborted = false;
    httpGetId = http->get(url.path(), tmpfile);
    connect(http, SIGNAL(requestFinished(int,bool)), this, SLOT(httpGetFinished(int, bool)));
    connect(http, SIGNAL(readyRead(QHttpResponseHeader)), this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
    qDebug() << "httpGetId: " << httpGetId;
  }
  else
    startInstaller();
}

void Updater::httpGetFinished(int requestId, bool error)
{
  if (requestId != httpGetId)
    return;
  
  tmpfile->close();
  if(error)
    tmpfile->remove();
  qDebug() << "Closed connection";
  checkForUpdate();
}

void Updater::readResponseHeader(const QHttpResponseHeader &responseHeader)
{
  if(responseHeader.statusCode() != 200)
  {
    http->abort();
    qDebug() << "response header:" << responseHeader.statusCode();
  }
}

void Updater::checkForUpdate()
{
  int pos = 0;
  updateAvailable = false;
    qDebug() << "Start checking for update";
    if (!tmpfile->open(QIODevice::ReadOnly | QIODevice::Text))
    {
	qDebug() << "Couldn't read update XML";
	return;
    }
    QXmlStreamReader xml(tmpfile);
    if(xml.hasError()) {
      qDebug() << "Has error" << xml.errorString(); 
    }
    while(!xml.atEnd())
    {
      xml.readNext();
      if(xml.isStartDocument())
	continue;
      if(xml.isStartElement() && xml.name() == "file")
      {
	QXmlStreamAttributes attrs = xml.attributes();
	versions.append(attrs.value("version").toString());
	kdevers.append(attrs.value("kdever").toString());
	hashes.append(attrs.value("hash").toString());
      }
      if(xml.isStartElement() && xml.name() == "changelog")
      {
	while(!xml.isEndElement())
	{
	  xml.readNext();
	  if(xml.isCDATA())
	    changeLogs.append(xml.text().toString());
	}
      }
    }
    xml.clear();
    tmpfile->close();
    tmpfile->remove();
    foreach (QString v, versions)
    {
      if((v.toDouble() > QString(VERSION).toDouble()) && 
	(kdevers.value(pos).toDouble() == QString(KDEVER).toDouble()))
      {
	latestVer = v;
	kdeBuildVer = kdevers.value(pos);
	md5sum = hashes.value(pos);
	changeLogText = changeLogs.value(pos);
	qDebug() << v << kdevers.value(pos);
	updateAvailable = true;
      }
      pos++;
    }
    if(updateAvailable)
      updateDialog();
    else
      startInstaller();
}

void Updater::updateDialog()
{
  UpdateNotification * notify = new UpdateNotification;
  notify->setCaption("Update Available");
  notify->setVersion(latestVer);
  notify->setMd5(md5sum);
  notify->setKdever(kdeBuildVer);
  notify->setChangeLog(changeLogText);
  notify->start();
}

UpdateNotification::UpdateNotification(QWidget* parent) : KDialog(parent)
{
  
}

UpdateNotification::~UpdateNotification()
{
}

void UpdateNotification::setVersion(QString v)
{
  latestVer = v;
}

void UpdateNotification::setMd5(QString h)
{
  md5sum = h;
}

void UpdateNotification::setChangeLog(QString c)
{
  changeLogText = c;
}

void UpdateNotification::setKdever(QString v)
{
  kdever = v;
}

QString UpdateNotification::getVersion()
{
  return latestVer;
}

QString UpdateNotification::getMd5()
{
  return md5sum;
}

QString UpdateNotification::getKdever()
{
  return kdever;
}

void UpdateNotification::startInstaller()
{
  delayedDestruct();
  PLDLiveInstaller * start = new PLDLiveInstaller;
  start->setCaption("PLD Live Installer");
  start->exec();
}

QWidget * UpdateNotification::start()
{
  setFixedSize(300,240);
  setButtons(User1 | Cancel);
  setButtonGuiItem(User1 , KGuiItem( i18n( "Download and Update" ), "task-recurring", i18n("Download and Update")));
  connect(this, SIGNAL(cancelClicked()), this, SLOT(startInstaller()));
  connect(this, SIGNAL(user1Clicked()), this, SLOT(getUpdate()));
  showButtonSeparator(true);

  KTitleWidget *titleWidget = new KTitleWidget(this);
  titleWidget->setText("<html><font size=\"3\">"
    "Version " + getVersion() + " is available for update.<br/>"
    "Release Notes:"
    "</font></html>");
  
  QTextEdit * changeLog = new QTextEdit;
  changeLog->setReadOnly(true);
  changeLog->insertHtml(changeLogText);
    
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(titleWidget);
  mainLayout->addWidget(changeLog);
  mainLayout->setMargin(0);

  QWidget *mainWidget = new QWidget;
  mainWidget->setLayout(mainLayout);
  mainWidget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

  setMainWidget(mainWidget);
  this->exec();
  return mainWidget;
}

void UpdateNotification::getUpdate()
{
  QString fileName = "/usr/sbin/pldliveinstaller";
  QString uurl = "http://pldliveinstaller.pld-linux.org/updates/" + getKdever() + "/" + getVersion() + "/pldliveinstaller";
  QUrl url(uurl);
  if (QFile::exists(fileName))
    QFile::remove(fileName);
  binfile = new QFile(fileName);
  if (binfile->isOpen())
    binfile->close();
  if (!binfile->open(QIODevice::WriteOnly)) {
    qDebug() << "Couldn't save installer binary file";
    delete binfile;
    binfile = 0;
    return;
  }
    
  http = new QHttp;
  http->setHost(url.host(), url.port(80));
  httpRequestAborted = false;
  httpGetId = http->get(url.path(), binfile);
  connect(http, SIGNAL(requestFinished(int,bool)), this, SLOT(httpGetFinished(int, bool)));
  connect(http, SIGNAL(readyRead(QHttpResponseHeader)), this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
  qDebug() << "httpGetId: " << httpGetId;
}

void UpdateNotification::httpGetFinished(int requestId, bool error)
{
  if (requestId != httpGetId)
    return;
  
  binfile->close();
  if(error)
  {
    binfile->remove();
    QMessageBox::information(this, "The update failed!",
			 "The update failed!"
			 "<br />No internet connection to update server!<br />",
			 QMessageBox::Close);
  }
  else
  {
    delayedDestruct();
    binfile->setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ReadOther);
    QMessageBox::information(this, "The update was successful!",
			 "The update was successful!"
			 "<br />Please restart the installer.",
			 QMessageBox::Close);
  }
  qDebug() << "Closed connection";
}

void UpdateNotification::readResponseHeader(const QHttpResponseHeader &responseHeader)
{
  if(responseHeader.statusCode() != 200)
  {
    http->abort();
    qDebug() << "response header:" << responseHeader.statusCode();
  }
}

#include "Updater.moc"