//===========================================
//  PC-BSD source code
//  Copyright (c) 2015, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details

#ifndef PORTLOOKUP_H
#define PORTLOOKUP_H
#include <QtCore>
#include <tuple>

namespace sysadm
{
struct PortInfo{
    int Port;
    QString Type;
    QString Keyword;
    QString Description;
    bool Recommended;
    friend bool operator<(const PortInfo lhs, const PortInfo rhs){
        return std::tie(lhs.Port,lhs.Type) < std::tie(rhs.Port,rhs.Type);
    }
    friend bool operator>(const PortInfo lhs, const PortInfo rhs)
    {   return rhs < lhs;}
    friend bool operator==(const PortInfo lhs, const PortInfo rhs)
    {
        return lhs.Port == rhs.Port && lhs.Type == rhs.Type;
    }
    friend bool operator !=(const PortInfo lhs, const PortInfo rhs)
    {   return !(lhs == rhs);}
};

const static QVector<int> recommendedPorts = {22, 80};
class Firewall
{

public:
    ///#section: ctors dtors
    Firewall();
    ~Firewall();
    ///#endsection

    ///#section: port commands
    /**
     * @description Returns a structure containing information about the port
     * including its port type, keyword, description, and whether it's a
     * recommended port
     *
     * @param number a port number between 0 and 2^16 - 1
     * @param type specify whether the port is tdp, udp, etc
     *
     * @ErrorConditions Port Number is set to -1 and a description of the error is stored in the description variable
     */
    PortInfo LookUpPort(int number, QString type);

    //Return all the known ports
    QList<PortInfo> allPorts();

    /**
     * @brief Opens a port
     * @param number a port number between 0 and 2^16 -1
     * @param type specify whether the port is tdp, udp, etc
     */
    void OpenPort(int number, QString type);

    /**
     * @brief Opens a set of ports
     * @param ports a vector of ports to open
     */
    void OpenPort(QList<PortInfo> ports);

    /**
     * @brief ClosePort closes a port
     * @param number a port number between 0 and 2^16 -1
     * @param type specify whether the port is tdp, udp, etc
     */
    void ClosePort(int number, QString type);

    /**
     * @brief ClosePort closes a set of ports
     * @param ports a vector of ports to close
     */
    void ClosePort(QList<PortInfo> ports);

    /**
     * @brief finds a list of ports that are open gets the info about them
     * and returns them
     * @return a QVector of the open ports
     */
    QList<PortInfo> OpenPorts();
    ///#endsection

    ///#section: firewall commands
    /**
     * @brief Checks to see if the firewall is running
     * @return true if the firewall is running, false if not
     */
    bool IsRunning();
    bool IsEnabled();
    /**
     * @brief Starts the firewall
     */
    void Start();
    /**
     * @brief Stops the firewall
     */
    void Stop();
    /**
     * @brief Restarts the firewall
     */
    void Restart();
    /**
     * @brief Enables the firewall in rc.conf, Start() will automatically enable the firewall
     */
    void Enable();
    /**
     * @brief Disables the firewall in rc.conf use after Stop() to completely disable
     */
    void Disable();
    /**
     * @brief Restores the Default Configuration
     */
    bool RestoreDefaults();
    ///#endsection

private:
    void readServicesFile();
    QStringList portStrings;

    QList<PortInfo> LoadOpenPorts();
    void SaveOpenPorts(QList<PortInfo> ports);

};
}
#endif // PORTLOOKUP_H
