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
//! @file   LogDataHandler.h
//! @author Flumes <flumes@lists.iei.liu.se>
//! @date   2012-12-18
//!
//! @brief Contains the LogData classes
//!
//$Id$

#ifndef LOGVARIABLE_H
#define LOGVARIABLE_H

#include <QSharedPointer>
#include <QVector>
#include <QMap>
#include <QString>
#include <QColor>
#include <QObject>

// Forward declaration
class LogVariableData;
class LogDataHandler;

typedef QSharedPointer<QVector<double> > SharedTimeVectorPtrT;
class UniqueSharedTimeVectorPtrHelper
{
public:
    SharedTimeVectorPtrT makeSureUnique(QVector<double> &rTimeVector);

private:
    QVector< SharedTimeVectorPtrT > mSharedTimeVecPointers;
};

QString makeConcatName(const QString componentName, const QString portName, const QString dataName);
void splitConcatName(const QString fullName, QString &rCompName, QString &rPortName, QString &rVarName);

//! @class VariableDescription
//! @brief Container class for strings describing a plot variable

class VariableDescription
{
public:
    enum VarTypeT {M, I, S, ST};
    QString mModelPath;
    QString mComponentName;
    QString mPortName;
    QString mDataName;
    QString mDataUnit;
    QString mAliasName;
    VarTypeT mVarType;

    QString getFullName() const;
    QString getFullNameWithSeparator(const QString sep) const;
    void setFullName(const QString compName, const QString portName, const QString dataName);

    QString getVarTypeString() const;

    bool operator==(const VariableDescription &other) const;
};
typedef QSharedPointer<VariableDescription> SharedVariableDescriptionT;

typedef QSharedPointer<LogVariableData> SharedLogVariableDataPtrT;

class LogVariableContainer : public QObject
{
    Q_OBJECT
public:
    typedef QMap<int, SharedLogVariableDataPtrT> GenerationMapT;

    //! @todo also need qucik constructor for creating a container with one generation directly
    LogVariableContainer(const SharedVariableDescriptionT &rVarDesc, LogDataHandler *pParentLogDataHandler);
    ~LogVariableContainer();
    void addDataGeneration(const int generation, const QVector<double> &rTime, const QVector<double> &rData);
    void addDataGeneration(const int generation, const SharedTimeVectorPtrT time, const QVector<double> &rData);
    void removeDataGeneration(const int generation);
    void removeGenerationsOlderThen(const int gen);
    void removeAllGenerations();

    SharedLogVariableDataPtrT getDataGeneration(const int gen=-1);
    bool hasDataGeneration(const int gen);
    int getLowestGeneration() const;
    int getHighestGeneration() const;
    int getNumGenerations() const;

    SharedVariableDescriptionT getVariableDescription() const;
    QString getAliasName() const;
    QString getFullVariableName() const;
    QString getFullVariableNameWithSeparator(const QString sep) const;
    QString getModelPath() const;
    QString getComponentName() const;
    QString getPortName() const;
    QString getDataName() const;
    QString getDataUnit() const;

    LogDataHandler *getLogDataHandler();

    void setAliasName(const QString alias);

signals:
    void nameChanged();

private:
    LogDataHandler *mpParentLogDataHandler;
    SharedVariableDescriptionT mVariableDescription;
    GenerationMapT mDataGenerations;
};
typedef QSharedPointer<LogVariableContainer> SharedLogVariableContainerPtrT;


class LogVariableData : public QObject
{
    Q_OBJECT

public:
    //! @todo maybe have protected constructor, to avoid creating these objects manually (need to be friend with container)
    LogVariableData(const int generation, const QVector<double> &rTime, const QVector<double> &rData, SharedVariableDescriptionT varDesc, LogVariableContainer *pParent);
    LogVariableData(const int generation, SharedTimeVectorPtrT time, const QVector<double> &rData, SharedVariableDescriptionT varDesc, LogVariableContainer *pParent);

    double mAppliedValueOffset;
    double mAppliedTimeOffset;
    int mGeneration;
    QVector<double> mDataVector;
    SharedTimeVectorPtrT mSharedTimeVectorPtr;

    const SharedVariableDescriptionT getVariableDescription() const;
    QString getAliasName() const;
    QString getFullVariableName() const;
    QString getFullVariableNameWithSeparator(const QString sep) const;
    QString getModelPath() const;
    QString getComponentName() const;
    QString getPortName() const;
    QString getDataName() const;
    QString getDataUnit() const;
    int getGeneration() const;
    int getLowestGeneration() const;
    int getHighestGeneration() const;
    int getNumGenerations() const;

    double getOffset() const;
    //double getScale() const;

    void addToData(const SharedLogVariableDataPtrT pOther);
    void addToData(const double other);
    void subFromData(const SharedLogVariableDataPtrT pOther);
    void subFromData(const double other);
    void multData(const SharedLogVariableDataPtrT pOther);
    void multData(const double other);
    void divData(const SharedLogVariableDataPtrT pOther);
    void divData(const double other);
    void assignToData(const SharedLogVariableDataPtrT pOther);
    bool pokeData(const int index, const double value);
    double peekData(const int index);

    LogDataHandler *getLogDataHandler();

public slots:
    void setValueOffset(double offset);
    void setTimeOffset(double offset);
    //setScale(double scale);


signals:
    void dataChanged();
    void nameChanged();

private:
    LogVariableContainer *mpParentVariableContainer;
    SharedVariableDescriptionT mpVariableDescription;
};


#endif // LOGVARIABLE_H
