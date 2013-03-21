#include "DiskListWidget.h"

#include <KDebug>

#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/StorageDrive>
#include <Solid/Block>
#include <Solid/StorageVolume>
	
DiskListWidget::DiskListWidget( QWidget *parent )
  : QListWidget( parent )
{

}

DiskListWidget::~DiskListWidget()
{
}

void DiskListWidget::refresh()
{
    //setCurrentItem(NULL);
    setSelectionMode(QAbstractItemView::SingleSelection);
    isBiggerThanThreeGB = false;
    
    if (diskListAlreadyOnList.size() <= 0)
    {
      devhash = new QHash<QString,Solid::Device>;
      parthash = new QHash<QString,Solid::Device>;
    }
    
    QStringList solidDevice;
    solidDevice.append(diskListAlreadyOnList);
    
    //get a list of all storage volumes
    QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::StorageDrive, QString());
    foreach (Solid::Device device, list)
    {
      if(device.is<Solid::StorageDrive>())
      {
	// show only devices bigger than 6GB
	//qDebug() << "TUTAJ!!" << device.as<Solid::StorageDrive>()->size()/1024/1024;
	//if(device.as<Solid::StorageDrive>()->size()/1024/1024 >= 6144)
	//{
	  
	  if(device.is<Solid::StorageVolume>())
	  {
	    if(device.as<Solid::StorageVolume>()->size()/1024/1024 >= 3072)
	      isBiggerThanThreeGB = true;
	  }
	  
	  QString dev = device.as<Solid::Block>()->device();
	
	  if (solidDevice.contains(dev))
	  {
	    solidDevice.removeOne(dev);
	  } 
	  else 
	  {
	
	    Solid::StorageDrive *harddisk = device.as<Solid::StorageDrive>();
	    if(((harddisk->driveType()==Solid::StorageDrive::HardDisk) || (harddisk->driveType()==Solid::StorageDrive::MemoryStick)))
	    {
	      QString text = device.product() + " (" + dev + ")"; 
	      QListWidgetItem *listitem = new QListWidgetItem;
	      //if ((harddisk->driveType()==Solid::StorageDrive::MemoryStick) || harddisk->bus()==Solid::StorageDrive::Usb)
		//listitem->setIcon(KIcon("drive-removable-media-usb").pixmap(64));
	      //else
		//listitem->setIcon(KIcon("drive-harddisk").pixmap(64));
	      listitem->setIcon(KIcon(device.icon()).pixmap(64));
	      listitem->setText(text);
	      listitem->setToolTip(dev);
	      addItem(listitem);
	      devhash->insert(text,device);
	      diskListAlreadyOnList.append(dev);
	      qDebug() << "Product: " << device.product() << "Vendor: " << device.vendor() 
		<< "Description: " << device.description() << "Dev: " << dev
		<< "Type: " << harddisk->driveType();
	  
	      foreach (Solid::Device partDev, Solid::Device::listFromType(Solid::DeviceInterface::Block, device.udi() ))
	      {
		QString part = partDev.as<Solid::Block>()->device();
		parthash->insert(part, partDev);
		//qDebug() << "has partition: " << part;
	      }
	  
	    }
	  }
	//} // devices bigger than 6GB
      }
    }
    foreach(QString soldev, solidDevice) {
      takeItem(row(findItems(".*"+soldev+".*",Qt::MatchRegExp)[0]));
      foreach(QString devDescr, diskListAlreadyOnList) {
	if (devDescr.contains(soldev))
	  diskListAlreadyOnList.removeAll(devDescr);
      }
    }
}

#include "DiskListWidget.moc"
