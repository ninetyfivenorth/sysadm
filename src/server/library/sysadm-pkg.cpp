//===========================================
//  PC-BSD source code
//  Copyright (c) 2015, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
#include "sysadm-general.h"
#include "sysadm-pkg.h"
#include "sysadm-global.h"
#include "globals.h"

using namespace sysadm;

// ==================
//  INLINE FUNCTIONS
// ==================
//Get Option Name
inline QString option_from_id(QString id){
  QSqlQuery q("SELECT option FROM option WHERE option_id = '"+id+"'");
    while(q.next()){ 
	return q.value("option").toString();
    }
  return ""; //nothing found
}
//Get Annotation (name/value - both use ID's)
inline QString anno_from_id(QString id){
  QSqlQuery q("SELECT annotation FROM annotation WHERE annotation_id = '"+id+"'");
    while(q.next()){ 
	return q.value("annotation").toString();
    }
  return ""; //nothing found
}
//Get origin from package_id (for reverse lookups)
inline QStringList origins_from_package_ids(QStringList ids){
  QSqlQuery q("SELECT origin FROM packages WHERE id = '"+ids.join("' OR id = '")+"'");
  QStringList out;
  while(q.next()){ out << q.value("origin").toString(); }
  return out;
}
//Generic ID's -> Names function (known databases: users, groups, licenses, shlibs, categories )
inline QStringList names_from_ids(QStringList ids, QString db){
  QSqlQuery q("SELECT name FROM "+db+" WHERE id = '"+ids.join("' OR id = '")+"'");
  QStringList out;
  while(q.next()){ out << q.value("name").toString(); }
  return out;
}
//provide values from ID's
inline QStringList provides_from_ids(QStringList ids){
  QSqlQuery q("SELECT provide FROM provides WHERE id = '"+ids.join("' OR id = '")+"'");
  QStringList out;
  while(q.next()){ out << q.value("provide").toString(); }
  return out;
}
//require values from ID's
inline QStringList requires_from_ids(QStringList ids){
  QSqlQuery q("SELECT require FROM requires WHERE id = '"+ids.join("' OR id = '")+"'");
  QStringList out;
  while(q.next()){ out << q.value("require").toString(); }
  return out;
}
inline QString getRepoFile(QString repo){
  if(repo=="local"){  return "/var/db/pkg/local.sqlite"; }
  else{ return ("/var/db/pkg/repo-"+repo+".sqlite"); }	
}
// =================
//  MAIN FUNCTIONS
// =================
QJsonObject PKG::pkg_info(QStringList origins, QString repo, QString category, bool fullresults){
  QJsonObject retObj;
  QString dbname = getRepoFile(repo);
  //qDebug() << "Database:" << dbname;// << conn;
   //Open the local database
  QSqlDatabase DB;
  if(QSqlDatabase::contains()){
    //database already loaded
    qDebug() << "Existing DB Connection";
    DB = QSqlDatabase::database();
  }else{
    //new database needs to be loaded
    qDebug() << "New DB Connection";
    DB = QSqlDatabase::addDatabase("QSQLITE");
    /*DB.setConnectOptions("QSQLITE_OPEN_READONLY=1");	  
    DB.setHostName("localhost");
    DB.setDatabaseName(dbname);*/
  }
  if(DB.databaseName()!=dbname){
    if(DB.isOpen()){ DB.close(); }
    DB.setConnectOptions("QSQLITE_OPEN_READONLY=1");	  
    DB.setHostName("localhost");
    DB.setDatabaseName(dbname);
  }
  qDebug() << "Open Database:" << DB.databaseName() << dbname;
    if( !DB.open() ){ 
	//qDebug() << " - could not be opened"; 
	return retObj; 
    } 
  //Now do all the pkg info, one pkg origin at a time
  origins.removeAll("");
    QString q_string = "SELECT * FROM packages";
    if(!origins.isEmpty()){ q_string.append(" WHERE origin = '"+origins.join("' OR origin = '")+"'"); }
    else if(!category.isEmpty()){ q_string.append(" WHERE origin LIKE '"+category+"/%'"); }
  QSqlQuery query(q_string);
    while(query.next()){
	QString id = query.value("id").toString(); //need this pkg id for later
	QString origin = query.value("origin").toString(); //need the origin for later
      if(id.isEmpty() || origin.isEmpty()){ continue; }
      QJsonObject info;
      //General info
      for(int i=0; i<query.record().count(); i++){
        info.insert(query.record().fieldName(i), query.value(i).toString() ); 
      }
      //ANNOTATIONS
      QSqlQuery q2("SELECT tag_id, value_id FROM pkg_annotation WHERE package_id = '"+id+"'");
      while(q2.next()){ 
	  //include the annotations as base-level fields as well
	  info.insert( anno_from_id(q2.value("tag_id").toString()), anno_from_id(q2.value("value_id").toString()) );
      }
      if(!fullresults){ retObj.insert(origin,info); continue; } //skip the rest of the info queries
      //OPTIONS
      QSqlQuery q3("SELECT option_id, value FROM pkg_option WHERE package_id = '"+id+"'");
      QJsonObject options;
      while(q3.next()){ options.insert(option_from_id(q3.value("option_id").toString()), q3.value("value").toString() ); }
      if(!options.isEmpty()){ info.insert("options",options); }
      //DEPENDENCIES
      QSqlQuery q4("SELECT origin FROM deps WHERE package_id = '"+id+"'");
      QStringList tmpList;
       while(q4.next()){ 
	  tmpList << q4.value("origin").toString();
      } //end deps query
      if(!tmpList.isEmpty()){ info.insert("dependencies", QJsonArray::fromStringList(tmpList) ); }
      //FILES
      QSqlQuery q5("SELECT path FROM files WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q5.next()){  tmpList << q5.value("path").toString(); }
      if(!tmpList.isEmpty()){ info.insert("files", QJsonArray::fromStringList(tmpList) ); }
      //REVERSE DEPENDENCIES
      QSqlQuery q6("SELECT package_id FROM deps WHERE origin = '"+origin+"'");
      tmpList.clear();
       while(q6.next()){ tmpList << q6.value("package_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("reverse_dependencies", QJsonArray::fromStringList(origins_from_package_ids(tmpList)) ); }
       //USERS
      QSqlQuery q7("SELECT user_id FROM pkg_users WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q7.next()){ tmpList << q7.value("user_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("users", QJsonArray::fromStringList(names_from_ids(tmpList, "users")) ); }
       //GROUPS
      QSqlQuery q8("SELECT group_id FROM pkg_groups WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q8.next()){ tmpList << q8.value("group_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("groups", QJsonArray::fromStringList(names_from_ids(tmpList, "users")) ); }
       //LICENSES
      QSqlQuery q9("SELECT license_id FROM pkg_licenses WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q9.next()){ tmpList << q9.value("license_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("licenses", QJsonArray::fromStringList(names_from_ids(tmpList, "licenses")) ); }
       //SHARED LIBS (REQUIRED)
      QSqlQuery q10("SELECT shlib_id FROM pkg_shlibs_required WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q10.next()){ tmpList << q10.value("shlib_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("shlibs_required", QJsonArray::fromStringList(names_from_ids(tmpList, "shlibs")) ); }
       //SHARED LIBS (PROVIDED)
      QSqlQuery q11("SELECT shlib_id FROM pkg_shlibs_provided WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q11.next()){ tmpList << q11.value("shlib_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("shlibs_provided", QJsonArray::fromStringList(names_from_ids(tmpList, "shlibs")) ); }
       //CONFLICTS
      QSqlQuery q12("SELECT conflict_id FROM pkg_conflicts WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q12.next()){ tmpList << q12.value("conflict_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("conflicts", QJsonArray::fromStringList(origins_from_package_ids(tmpList)) ); }
      //CONFIG FILES
      QSqlQuery q13("SELECT path FROM config_files WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q13.next()){  tmpList << q13.value("path").toString(); }
      if(!tmpList.isEmpty()){ info.insert("config_files", QJsonArray::fromStringList(tmpList) ); }
      //PROVIDES
      QSqlQuery q14("SELECT provide_id FROM pkg_provides WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q14.next()){ tmpList << q14.value("provide_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("provides", QJsonArray::fromStringList(provides_from_ids(tmpList)) ); }
      //REQUIRES
      QSqlQuery q15("SELECT require_id FROM pkg_requires WHERE package_id = '"+id+"'");
      tmpList.clear();
       while(q15.next()){ tmpList << q15.value("require_id").toString(); }
       if(!tmpList.isEmpty()){ info.insert("requires", QJsonArray::fromStringList(requires_from_ids(tmpList)) ); }
       //Now insert this information into the main object
       retObj.insert(origin,info);
  } //end loop over pkg matches
  DB.close();
  return retObj;
}

QStringList PKG::pkg_search(QString repo, QString searchterm, QString category){
  QStringList found;
  QString dbname = getRepoFile(repo);
  //qDebug() << "Database:" << dbname;// << conn;
   //Open the local database
  QSqlDatabase DB;
  if(QSqlDatabase::contains()){
    //database already loaded
    qDebug() << "Existing DB Connection";
    DB = QSqlDatabase::database();
  }else{
    //new database needs to be loaded
    qDebug() << "New DB Connection";
    DB = QSqlDatabase::addDatabase("QSQLITE");
    /*DB.setConnectOptions("QSQLITE_OPEN_READONLY=1");	  
    DB.setHostName("localhost");
    DB.setDatabaseName(dbname);*/
  }
  if(DB.databaseName()!=dbname){
    if(DB.isOpen()){ DB.close(); }
    DB.setConnectOptions("QSQLITE_OPEN_READONLY=1");	  
    DB.setHostName("localhost");
    DB.setDatabaseName(dbname);
  }
  qDebug() << "Open Database:" << DB.databaseName() << dbname;
    if( !DB.open() ){ 
	//qDebug() << " - could not be opened"; 
	return found; 
    } 
  QString q_string = "SELECT origin FROM packages WHERE name LIKE '"+searchterm+"%'";
    if(!category.isEmpty()){ q_string.append(" AND origin LIKE '"+category+"/%'"); }
    QSqlQuery query(q_string);
    while(query.next()){
	found << query.value("origin").toString(); //need the origin for later
    }
  if(found.isEmpty()){
    //Expand the search to names containing the term
    q_string = "SELECT origin FROM packages WHERE name LIKE '%"+searchterm+"%'";
    if(!category.isEmpty()){ q_string.append(" AND origin LIKE '"+category+"/%'"); }
    QSqlQuery q2(q_string);
    while(q2.next()){
	found << q2.value("origin").toString(); //need the origin for later
    }
  }
  if(found.isEmpty()){
    //Expand the search to comments
    q_string = "SELECT origin FROM packages WHERE comment LIKE '%"+searchterm+"%'";
    if(!category.isEmpty()){ q_string.append(" AND origin LIKE '"+category+"/%'"); }
    QSqlQuery q2(q_string);
    while(q2.next()){
	found << q2.value("origin").toString(); //need the origin for later
    }
  }
  if(found.isEmpty()){
    //Expand the search to full descriptions
    q_string = "SELECT origin FROM packages WHERE desc LIKE '%"+searchterm+"%'";
    if(!category.isEmpty()){ q_string.append(" AND origin LIKE '"+category+"/%'"); }
    QSqlQuery q2(q_string);
    while(q2.next()){
	found << q2.value("origin").toString(); //need the origin for later
    }
  }
  DB.close();
  return found;
}

QJsonArray PKG::list_categories(QString repo){
  QString dbname = getRepoFile(repo);
  //qDebug() << "Database:" << dbname;// << conn;
   //Open the local database
  QSqlDatabase DB;
  if(QSqlDatabase::contains()){
    //database already loaded
    qDebug() << "Existing DB Connection";
    DB = QSqlDatabase::database();
  }else{
    //new database needs to be loaded
    qDebug() << "New DB Connection";
    DB = QSqlDatabase::addDatabase("QSQLITE");
    /*DB.setConnectOptions("QSQLITE_OPEN_READONLY=1");	  
    DB.setHostName("localhost");
    DB.setDatabaseName(dbname);*/
  }
  if(DB.databaseName()!=dbname){
    if(DB.isOpen()){ DB.close(); }
    DB.setConnectOptions("QSQLITE_OPEN_READONLY=1");	  
    DB.setHostName("localhost");
    DB.setDatabaseName(dbname);
  }
  qDebug() << "Open Database:" << DB.databaseName() << dbname;
    if( !DB.open() ){ 
	//qDebug() << " - could not be opened"; 
	return QJsonArray(); 
    } 
  //Get all the pkg origins for this repo
  QStringList origins;
    QSqlQuery q_o("SELECT origin FROM packages");
    while(q_o.next()){
	origins << q_o.value("origin").toString(); //need the origin for later
    }
  //Now get all the categories
  QString q_string = "SELECT name FROM categories";
    QSqlQuery query(q_string);
    QStringList found;
    while(query.next()){
	found << query.value("name").toString(); //need the origin for later
    }
    //Now check all the categories to ensure that pkgs exist within it
    for(int i=0; i<found.length(); i++){
      if(origins.filter(found[i]+"/").isEmpty()){ found.removeAt(i); i--; }
    }
    if(!found.isEmpty()){ return QJsonArray::fromStringList(found); }
    else{ return QJsonArray(); }
}

QJsonArray PKG::list_repos(){
  QString dbdir = "/var/db/pkg/repo-%1.sqlite";
  QDir confdir("/usr/local/etc/pkg/repos");
    QStringList confs = confdir.entryList(QStringList() << "*.conf", QDir::Files);
  QStringList found;
  found << "local"; //There is always a local database (for installed pkgs)
  for(int i=0; i<confs.length(); i++){
    QStringList repoinfo = General::readTextFile(confdir.absoluteFilePath(confs[i])).join("\n").split("}");
    for(int j=0; j<repoinfo.length(); j++){
      QString repo = repoinfo[j].section(":",0,0);
      if(QFile::exists(dbdir.arg(repo))){ found << repo; }
    }
  }
  return QJsonArray::fromStringList(found);
}

//=================
//pkg modification routines (dispatcher events for notifications)
//=================
QJsonObject PKG::pkg_install(QStringList origins, QString repo){
  //Generate the command to run
  QString cmd = "pkg install -y %1";
  if(!repo.isEmpty() && repo!="local"){ cmd = cmd.arg("--repository \""+repo+"\" %1"); }
  cmd = cmd.arg( origins.join(" ") );
  //Now kick off the dispatcher process (within the pkg queue - since only one pkg process can run at a time)
  QString ID = "sysadm_pkg_install-"+QUuid::createUuid().toString(); //create a random tag for the process
  DISPATCHER->queueProcess(Dispatcher::PKG_QUEUE, ID, cmd);
  //Now return the info about the process
  QJsonObject obj;
    obj.insert("status", "pending");
    obj.insert("proc_cmd",cmd);
    obj.insert("proc_id",ID);
  return obj;
}

QJsonObject PKG::pkg_remove(QStringList origins, bool recursive){
  //Generate the command to run
  QString cmd = "pkg delete -y %1";
  if(recursive){ cmd = cmd.arg("-R %1"); } //also remove all packages which depend on these pkgs
  cmd = cmd.arg( origins.join(" ") );
  //Now kick off the dispatcher process (within the pkg queue - since only one pkg process can run at a time)
  QString ID = "sysadm_pkg_remove-"+QUuid::createUuid().toString(); //create a random tag for the process
  DISPATCHER->queueProcess(Dispatcher::PKG_QUEUE, ID, cmd);
  //Now return the info about the process
  QJsonObject obj;
    obj.insert("status", "pending");
    obj.insert("proc_cmd",cmd);
    obj.insert("proc_id",ID);
  return obj;	
}

QJsonObject PKG::pkg_lock(QStringList origins){
  //Generate the command to run
  QString cmd = "pkg lock -y %1";
  cmd = cmd.arg( origins.join(" ") );
  //Now kick off the dispatcher process (within the pkg queue - since only one pkg process can run at a time)
  QString ID = "sysadm_pkg_lock-"+QUuid::createUuid().toString(); //create a random tag for the process
  DISPATCHER->queueProcess(Dispatcher::PKG_QUEUE, ID, cmd);
  //Now return the info about the process
  QJsonObject obj;
    obj.insert("status", "pending");
    obj.insert("proc_cmd",cmd);
    obj.insert("proc_id",ID);
  return obj;	
}

QJsonObject PKG::pkg_unlock(QStringList origins){
  //Generate the command to run
  QString cmd = "pkg unlock -y %1";
  cmd = cmd.arg( origins.join(" ") );
  //Now kick off the dispatcher process (within the pkg queue - since only one pkg process can run at a time)
  QString ID = "sysadm_pkg_unlock-"+QUuid::createUuid().toString(); //create a random tag for the process
  DISPATCHER->queueProcess(Dispatcher::PKG_QUEUE, ID, cmd);
  //Now return the info about the process
  QJsonObject obj;
    obj.insert("status", "pending");
    obj.insert("proc_cmd",cmd);
    obj.insert("proc_id",ID);
  return obj;		
}

//==================
//pkg administration routines
//==================
QJsonObject PKG::pkg_update(bool force){
  //Generate the command to run
  QString cmd = "pkg update";
  if(force){ cmd.append(" -f"); }
  //Now kick off the dispatcher process (within the pkg queue - since only one pkg process can run at a time)
  QString ID = "sysadm_pkg_update-"+QUuid::createUuid().toString(); //create a random tag for the process
  DISPATCHER->queueProcess(Dispatcher::PKG_QUEUE, ID, cmd);
  //Now return the info about the process
  QJsonObject obj;
    obj.insert("status", "pending");
    obj.insert("proc_cmd",cmd);
    obj.insert("proc_id",ID);
  return obj;	
}

QJsonObject PKG::pkg_check_upgrade(){
  //Generate the command to run
  QString cmd = "pkg upgrade -n";
  //Now kick off the dispatcher process (within the pkg queue - since only one pkg process can run at a time)
  QString ID = "sysadm_pkg_check_upgrade-"+QUuid::createUuid().toString(); //create a random tag for the process
  DISPATCHER->queueProcess(Dispatcher::PKG_QUEUE, ID, cmd);
  //Now return the info about the process
  QJsonObject obj;
    obj.insert("status", "pending");
    obj.insert("proc_cmd",cmd);
    obj.insert("proc_id",ID);
  return obj;	
}

QJsonObject PKG::pkg_upgrade(){
  //Generate the command to run
  QString cmd = "pkg upgrade -y";
  //Now kick off the dispatcher process (within the pkg queue - since only one pkg process can run at a time)
  QString ID = "sysadm_pkg_upgrade-"+QUuid::createUuid().toString(); //create a random tag for the process
  DISPATCHER->queueProcess(Dispatcher::PKG_QUEUE, ID, cmd);
  //Now return the info about the process
  QJsonObject obj;
    obj.insert("status", "pending");
    obj.insert("proc_cmd",cmd);
    obj.insert("proc_id",ID);
  return obj;
}

QJsonObject PKG::pkg_audit(){
  //Generate the command to run
  QString cmd = "pkg audit -qr";
  //Now kick off the dispatcher process (within the pkg queue - since only one pkg process can run at a time)
  QString ID = "sysadm_pkg_audit-"+QUuid::createUuid().toString(); //create a random tag for the process
  DISPATCHER->queueProcess(Dispatcher::PKG_QUEUE, ID, cmd);
  //Now return the info about the process
  QJsonObject obj;
    obj.insert("status", "pending");
    obj.insert("proc_cmd",cmd);
    obj.insert("proc_id",ID);
  return obj;	
}
