#include <KApplication>
#include <KCmdLineArgs>
#include <KAboutData>
#include <KLocale>
#include "PLDLiveInstaller.h"
#include "Updater.h"
#include "Version.h"

void fillAboutData(KAboutData& aboutData);

int main(int argc, char** argv)
{
  
    KAboutData aboutData( "PLDLiveInstaller", "PLD Live Installer", ki18n(VERSION_STR),
			  VERSION, ki18n("Live Installer"), KAboutData::License_GPL);
			  
    fillAboutData(aboutData);
  
    KCmdLineArgs::init( argc, argv, &aboutData );
  
    KApplication app;
    //PLDLiveInstaller * start = new PLDLiveInstaller();
    //start->setCaption("PLD Live Installer");
    //start->show();
    Updater * update = new Updater;
    update->getLatestVersion();
    
    return app.exec();
}

void fillAboutData(KAboutData& aboutData)
{
  aboutData.setProgramIconName("tools-wizard");
  aboutData.addAuthor(ki18n("Bartosz Świątek"), ki18n("Maintainer, Qt4"), "shadzik@pld-linux.org");
}