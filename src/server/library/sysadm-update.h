//===========================================
//  PC-BSD source code
//  Copyright (c) 2015, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#ifndef __PCBSD_LIB_UTILS_UPDATER_H
#define __PCBSD_LIB_UTILS_UPDATER_H

#include <QJsonObject>
#include "sysadm-global.h"

namespace sysadm{

class Update{
public:
	//Time stamps
	static QDateTime lastFullCheck();
	static QDateTime rebootRequiredSince();
	//Listing routines
	static QJsonObject checkUpdates(bool fast = false);
	static void saveCheckUpdateLog(QString);  //Internal for Dispatcher process usage - do not expose to public API

	static QJsonObject listBranches();
	//Start/stop update routine
	static QJsonObject startUpdate(QJsonObject);
	static QJsonObject stopUpdate();
	static QJsonObject applyUpdates();
	//Read/write update settings
	static QJsonObject readSettings();
	static QJsonObject writeSettings(QJsonObject);
	//List/Read update logs
	static QJsonObject listLogs();
	static QJsonObject readLog(QJsonObject);

};

}

#endif
