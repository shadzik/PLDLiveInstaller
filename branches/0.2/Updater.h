#ifndef UPDATER_H
#define UPDATER_H

#include <KDialog>

#include <QProgressBar>
#include <QFile>
#include <QHttp>
#include <QHttpResponseHeader>
#include <QList>
#include <QXmlStreamReader>
#include <QMap>

class Updater : QObject
{
  Q_OBJECT
  
  QProgressBar * updateBar;
  void checkForUpdate();
  
  public:
    void getLatestVersion();
    Updater( QWidget * parent = 0 );
    virtual ~Updater();
  
  public Q_SLOTS:
    void startInstaller();
    void httpGetFinished(int requestId, bool error);
    void readResponseHeader(const QHttpResponseHeader &responseHeader);
    
  private:
    QHttp * http;
    QFile * tmpfile;
    QStringList versions, kdevers, hashes, changeLogs;
    QString latestVer, kdeBuildVer, md5sum, changeLogText;
    int httpGetId;
    bool httpRequestAborted, updateAvailable;
    void updateDialog();
    bool shouldWeCheckForUpdate();
  
};

class UpdateNotification : public KDialog
{
  Q_OBJECT
  
  public:
    UpdateNotification(QWidget* parent = 0);
    virtual ~UpdateNotification();
    void setVersion(QString);
    void setMd5(QString);
    void setChangeLog(QString);
    void setKdever(QString);
    QString getVersion();
    QString getMd5();
    QString getKdever();
    QWidget * start();
    
  public Q_SLOTS:
    void startInstaller();
    void getUpdate();
    void httpGetFinished(int requestId, bool error);
    void readResponseHeader(const QHttpResponseHeader &responseHeader);
    
  private:
    QString latestVer, md5sum, changeLogText, kdever;
    QHttp *http;
    QFile * binfile;
    int httpGetId;
    bool httpRequestAborted;
};

#endif // UPDATER_H
