/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2011 
    Mikael Axin, Robert Braun, Alessandro Dell'Amico, Björn Eriksson,
    Peter Nordin, Karl Pettersson, Petter Krus, Ingo Staack

 This file is provided "as is", with no guarantee or warranty for the
 functionality or reliability of the contents. All contents in this file is
 the original work of the copyright holders at the Division of Fluid and
 Mechatronic Systems (Flumes) at Linköping University. Modifying, using or
 redistributing any part of this file is prohibited without explicit
 permission from the copyright holders.
-----------------------------------------------------------------------------*/

//!
//! @file   CoreAccess.h
//! @author Flumes <flumes@lists.iei.liu.se>
//! @date   2010-01-01
//!
//! @brief Contains the CoreAccess class, The API class for communication with the HopsanCore
//!
//$Id$

#ifndef GUIROOTSYSTEM_H
#define GUIROOTSYSTEM_H

#include "common.h"

#include <QString>
#include <qdebug.h>

//Forward declaration of hopsan core classes
namespace hopsan {
class ComponentSystem;
class Port;
}


class CoreLibraryAccess
{
public:
    bool hasComponent(QString componentName);
    bool loadComponentLib(QString fileName);
    bool unLoadComponentLib(QString fileName);
};

class CoreMessagesAccess
{
public:
    size_t getNumberOfMessages();
    void getMessage(QString &rMessage, QString &rType, QString &rTag);
};

class CoreSystemAccess
{
public:
    enum PortTypeIndicatorT {INTERNALPORTTYPE, ACTUALPORTTYPE, EXTERNALPORTTYPE};

    CoreSystemAccess(QString name=QString(), CoreSystemAccess* pParentCoreSystemAccess=0);
    ~CoreSystemAccess();
    hopsan::ComponentSystem *getCoreSubSystemPtr(QString name);
    void deleteRootSystemPtr(); //!< @todo This is very strange, needed becouse core systems are deleted from parent if they are subsystems (not if root systems), this is the only way to safely delete the core object

    bool connect(QString compname1, QString portname1, QString compname2, QString portname2);
    bool disconnect(QString compname1, QString portname1, QString compname2, QString portname2);

    void setDesiredTimeStep(double timestep);
    void setInheritTimeStep(bool inherit);
    bool doesInheritTimeStep();
    double getDesiredTimeStep();

    //! @todo maybe we should use name="" (empty) to indicate root system instead, to cut down on the number of functions
    QString getRootSystemTypeCQS();
    QString getSubComponentTypeCQS(QString componentName);

    QString setRootSystemName(QString name);
    QString getRootSystemName();

    QString renameSubComponent(QString componentName, QString name);

    QString getPortType(const QString componentName, const QString portName, const PortTypeIndicatorT portTypeIndicator=ACTUALPORTTYPE);
    QString getNodeType(QString componentName, QString portName);

    void getStartValueDataNamesValuesAndUnits(QString componentName, QString portName, QVector<QString> &rNames, QVector<double> &rStartDataValues, QVector<QString> &rUnits);
    void getStartValueDataNamesValuesAndUnits(QString componentName, QString portName, QVector<QString> &rNames, QVector<QString> &rStartDataValuesTxt, QVector<QString> &rUnits);
//    bool setStartValueDataByNames(QString componentName, QString portName, QVector<QString> names, QVector<double> startDataValues);
//    bool setStartValueDataByNames(QString componentName, QString portName, QVector<QString> names, QVector<QString> startDataValues);

    void getParameters(QString componentName, QVector<QString> &qParameterNames, QVector<QString> &qParameterValues, QVector<QString> &qDescriptions, QVector<QString> &qUnits, QVector<QString> &qTypes);
    QStringList getParameterNames(QString componentName);
    QString getParameterUnit(QString componentName, QString parameterName);
    QString getParameterDescription(QString componentName, QString parameterName);
    QString getParameterValue(QString componentName, QString parameterName);
//    QString getParameterValueTxt(QString componentName, QString parameterName);
    bool setParameter(QString componentName, QString parameterName, QString value, bool force=0); //!< @todo maybe call this set parameter value
 //   bool setParameter(QString componentName, QString parameterName, QString sysParName);

    QString createComponent(QString type, QString name="");
    QString createSubSystem(QString name="");
    void removeSubComponent(QString componentName, bool doDelete);

    bool isSimulationOk();
    bool initialize(double mStartTime, double mFinishTime, int nSamples=2048);
    void simulate(double mStartTime, double mFinishTime, simulationMethod type, size_t nThreads = 0);
    void simulate(double mStartTime, double mFinishTime);
    void finalize(double mStartTime, double mFinishTime);
    double getCurrentTime();
    void stop();

    void deleteSystemPort(QString portname);
    QString addSystemPort(QString portname);
    QString renameSystemPort(QString oldname, QString newname);

    QString reserveUniqueName(QString desiredName);
    void unReserveUniqueName(QString name);

    bool setSystemParameter(QString name, QString value, QString description="", QString unit="", QString type="", bool force=false);
    QString getSystemParameter(QString name);
    bool hasSystemParameter(QString name);
    void removeSystemParameter(QString name);
    QMap<std::string, std::string> getSystemParametersMap();
    void getSystemParameters(QVector<QString> &qParameterNames, QVector<QString> &qParameterValues, QVector<QString> &qDescriptions, QVector<QString> &qUnits, QVector<QString> &qTypes);

    std::vector<double> getTimeVector(QString componentName, QString portName);
    void getPlotDataNamesAndUnits(const QString compname, const QString portname, QVector<QString> &rNames, QVector<QString> &rUnits);
    QString getPlotDataUnit(const QString compname, const QString portname, const QString dataname);
    //void getPlotDataUnit(const QString compname, const QString portname, const string dataname, QString &rUnit);
    //QVector<QString> getPlotDataUnits();
    void getPlotData(const QString compname, const QString portname, const QString dataname, QPair<QVector<double>, QVector<double> > &rData);
    bool getLastNodeData(const QString compname, const QString portname, const QString dataname, double& rData);
    bool isPortConnected(QString componentName, QString portName);

private:
    hopsan::Port* getPortPtr(QString componentName, QString portName);

    //*****Core Interaction*****
    hopsan::ComponentSystem *mpCoreComponentSystem;
    //**************************
};

#endif // GUIROOTSYSTEM_H
