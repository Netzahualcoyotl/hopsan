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
//! @file   OptimizationWorkerParticleSwarm.cpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2014-02-13
//! @version $Id: OptimizationHandler.cpp 6525 2014-01-30 15:58:59Z petno25 $
//!
//! @brief Contains an optimization worker object for the particle swarm algorithm
//!

//Hopsan includes
#include "OptimizationWorkerParticleSwarm.h"
#include "OptimizationWorker.h"
#include "OptimizationHandler.h"
#include "LogDataHandler.h"
#include "Widgets/ModelWidget.h"
#include "Widgets/HcomWidget.h"
#include "HcomHandler.h"
#include "Configuration.h"
#include "GUIObjects/GUIContainerObject.h"
#include "PlotHandler.h"
#include "PlotTab.h"
#include "global.h"
#include "PlotWindow.h"
#include "MainWindow.h"
#include "Dialogs/OptimizationDialog.h"
#include "ModelHandler.h"

//C++ includes
#include <math.h>

//! @brief Initializes a particle swarm optimization
OptimizationWorkerParticleSwarm::OptimizationWorkerParticleSwarm(OptimizationHandler *pHandler)
    : OptimizationWorker(pHandler)
{
    mPrintLogOutput = true;  //! @todo Should be changeable by user
}

void OptimizationWorkerParticleSwarm::init()
{
    OptimizationWorker::init();

    for(int p=0; p<mNumPoints; ++p)
    {
        mParameters[p].resize(mNumParameters);
        mVelocities[p].resize(mNumParameters);
        for(int i=0; i<mNumParameters; ++i)
        {
            //Initialize points
            double r = double(rand()) / double(RAND_MAX);
            mParameters[p][i] = mParMin[i] + r*(mParMax[i]-mParMin[i]);
            if(mpHandler->mParameterType == OptimizationHandler::Integer)
            {
                mParameters[p][i] = round(mParameters[p][i]);
            }

            //Initialize velocities
            double minVel = -fabs(mParMax[i]-mParMin[i]);
            double maxVel = fabs(mParMax[i]-mParMin[i]);
            r = double(rand()) / double(RAND_MAX);
            mVelocities[p][i] = minVel + r*(maxVel-minVel);
        }
    }
    mObjectives.resize(mNumPoints);

    LogDataHandler *pHandler = mModelPtrs[0]->getViewContainerObject()->getLogDataHandler();
    // Check if exist at any generation first to avoid error message
    if (pHandler->hasVariable("WorstObjective"))
    {
        pHandler->deleteVariable("WorstObjective");
    }
    if (pHandler->hasVariable("BestObjective"))
    {
        pHandler->deleteVariable("BestObjective");
    }

    // Close these plotwindows before optimization to make sure old data is removed
    //! @todo should have define or const for this name "parplot"
    PlotWindow *pPlotWindow = gpPlotHandler->getPlotWindow("parplot");
    if(pPlotWindow)
    {
        PlotTab *pPlotTab = pPlotWindow->getCurrentPlotTab();
        if(pPlotTab)
        {
            while(!pPlotTab->getCurves().isEmpty())
            {
                pPlotTab->removeCurve(pPlotTab->getCurves().first());
            }
        }
    }
}


//! @brief Executes a particle swarm algorithm. optParticleInit() must be called before this one.
void OptimizationWorkerParticleSwarm::run()
{
    plotPoints();

    mpHandler->mpConsole->mpTerminal->setEnabledAbortButton(true);

    mConvergenceReason=0;

   if(!mpHandler->mpHcomHandler->hasFunction("evalall"))
    {
        printError("Function \"evalall\" not defined.","",false);
        return;
    }

    mpHandler->mpConsole->print("Running optimization...");

    //Disable terminal output during optimization
    execute("echo off");

    //Evaluate initial objevtive values
    execute("call evalall");
    if(mpHandler->mpHcomHandler->getVar("ans") == -1)    //This check is needed if abort key is pressed while evaluating
    {
        execute("echo on");
        mpHandler->mpConsole->print("Optimization aborted.");
        //mpHcomHandler->setModelPtr(qobject_cast<ModelWidget*>(gpCentralTabWidget->currentWidget()));
        finalize();
        return;
    }

    //Initialize best known point for each point
    for(int i=0; i<mNumPoints; ++i)
    {
        mBestKnowns[i] = mParameters[i];
        mBestObjectives[i] = mObjectives[i];
    }

    //Calculate best known global position
    calculateBestAndWorstId();
    mPsBestObj = mObjectives[mBestId];
    mBestPoint = mParameters[mBestId];

    int i=0;
    int percent=-1;
    for(; i<mMaxEvals && !mpHandler->mpHcomHandler->isAborted(); ++i)
    {
        //Process events, to make sure GUI is updated
        qApp->processEvents();

        //Abort if abort key was pressed
        if(mpHandler->mpHcomHandler->isAborted())
        {
            mpHandler->mpConsole->print("Optimization aborted.");
            //gpModelHandler->setCurrentModel(qobject_cast<ModelWidget*>(gpCentralTabWidget->currentWidget()));
            finalize();
            return;
        }

        //Print log output
        printLogOutput();

        //Update progress bar in dialog
        updateProgressBar(i);

        //Move particles
        moveParticles();

        //Evaluate objevtive values
        if(mpHandler->mpConfig->getUseMulticore())
        {
            //Multi-threading, we cannot use the "evalall" function
            for(int i=0; i<mNumPoints && !mpHandler->mpHcomHandler->isAborted(); ++i)
            {
                mpHandler->mpHcomHandler->setModelPtr(mModelPtrs[i]);
                execute("opt set evalid "+QString::number(i));
                execute("call setpars");
            }
            gpModelHandler->simulateMultipleModels_blocking(mModelPtrs); //Ok to use global model handler for this, it does not use any member stuff
            for(int i=0; i<mNumPoints && !mpHandler->mpHcomHandler->isAborted(); ++i)
            {
                mpHandler->mpHcomHandler->setModelPtr(mModelPtrs[i]);
                execute("opt set evalid "+QString::number(i));
                execute("call obj");
            }
            mpHandler->mpHcomHandler->setModelPtr(mModelPtrs.first());
        }
        else
        {
            execute("call evalall");
        }
        if(mpHandler->mpHcomHandler->getVar("ans") == -1 || mpHandler->mpHcomHandler->isAborted())    //This check is needed if abort key is pressed while evaluating
        {
            execute("echo on");
            mpHandler->mpConsole->print("Optimization aborted.");
           //mpHcomHandler->setModelPtr(qobject_cast<ModelWidget*>(gpCentralTabWidget->currentWidget()));
            finalize();
            return;
        }

        //Calculate best known positions
        for(int p=0; p<mNumPoints; ++p)
        {
            if(mObjectives[p] < mBestObjectives[p])
            {
                mBestKnowns[p] = mParameters[p];
                mBestObjectives[p] = mObjectives[p];
            }
        }

        //Calculate best known global position
        calculateBestAndWorstId();
        if(mObjectives[mBestId] < mPsBestObj)
        {
            mPsBestObj = mObjectives[mBestId];
            mBestPoint = mParameters[mBestId];
        }
        gpMainWindow->mpOptimizationDialog->updateParameterOutputs(mObjectives, mParameters, mBestId, mWorstId);

        plotPoints();
        plotObjectiveFunctionValues();

        //Check convergence
        if(checkForConvergence()) break;      //Use complex method, it's the same principle
    }

    execute("echo on");

    switch(mConvergenceReason)
    {
    case 0:
        mpHandler->mpConsole->print("Optimization failed to converge after "+QString::number(i)+" iterations.");
        break;
    case 1:
        mpHandler->mpConsole->print("Optimization converged in function values after "+QString::number(i)+" iterations.");
        break;
    case 2:
        mpHandler->mpConsole->print("Optimization converged in parameter values after "+QString::number(i)+" iterations.");
        break;
    }

    mTotalIterations = i;

    mpHandler->mpConsole->print("\nBest point:");
    for(int i=0; i<mNumParameters; ++i)
    {
        mpHandler->mpConsole->print("par("+QString::number(i)+"): "+QString::number(mParameters[mBestId][i]));
    }

    //Clean up
    finalize();
}

void OptimizationWorkerParticleSwarm::finalize()
{
    OptimizationWorker::finalize();
}



//! @brief Moves the particles (for particle swarm optimization)
void OptimizationWorkerParticleSwarm::moveParticles()
{
    for (int p=0; p<mNumPoints; ++p)
    {
        double r1 = double(rand())/double(RAND_MAX);
        double r2 = double(rand())/double(RAND_MAX);
        for(int j=0; j<mNumParameters; ++j)
        {
            mVelocities[p][j] = mPsOmega*mVelocities[p][j] + mPsC1*r1*(mBestKnowns[p][j]-mParameters[p][j]) + mPsC2*r2*(mBestPoint[j]-mParameters[p][j]);
            mParameters[p][j] = mParameters[p][j]+mVelocities[p][j];
            if(mParameters[p][j] <= mParMin[j])
            {
                mParameters[p][j] = mParMin[j];
                mVelocities[p][j] = 0.0;
            }
            if(mParameters[p][j] >= mParMax[j])
            {
                mParameters[p][j] = mParMax[j];
                mVelocities[p][j] = 0.0;
            }
        }
    }
}



//! @brief Prints logging output about particles (for use with particle swarm algorithm)
//! @todo Extent so it can also be used with complex algorithm
void OptimizationWorkerParticleSwarm::printLogOutput()
{
    if(mPrintLogOutput)
    {
        if(mLogOutput.isEmpty())
        {
            //Prepare logging output list
            for(int p=0; p<mNumPoints; ++p)
            {
                mLogOutput.append("Particle    "+QString::number(p)+":\n");
                mLogOutput[p].append("Position: \t\t\tVelocity: \t\t\tLocal best: \t\t\tGlobal best:\n");

            }
        }

        for(int p=0; p<mNumPoints; ++p)
        {
            for(int i=0; i<mParameters[p].size(); ++i)
            {
                mLogOutput[p].append(QString::number(mParameters[p][i])+",");
            }
            mLogOutput[p].chop(1);
            mLogOutput[p].append("\t\t");
            for(int i=0; i<mVelocities[p].size(); ++i)
            {
                mLogOutput[p].append(QString::number(mVelocities[p][i])+",");
            }
            mLogOutput[p].chop(1);
            mLogOutput[p].append("\t\t");
            for(int i=0; i<mBestKnowns[p].size(); ++i)
            {
                mLogOutput[p].append(QString::number(mBestKnowns[p][i])+",");
            }
            mLogOutput[p].chop(1);
            mLogOutput[p].append("\t\t");
            for(int i=0; i<mBestPoint.size(); ++i)
            {
                mLogOutput[p].append(QString::number(mBestPoint[i])+",");
            }
            mLogOutput[p].chop(1);
            mLogOutput[p].append("\n");
        }
    }
}


void OptimizationWorkerParticleSwarm::setOptVar(const QString &var, const QString &value)
{
    OptimizationWorker::setOptVar(var, value);

    if(var == "npoints")
    {
        int n = value.toInt();
        mVelocities.resize(n);
        mBestKnowns.resize(n);
        mBestObjectives.resize(n);
    }
    else if(var == "nparams")
    {
        int n = value.toInt();
        mBestPoint.resize(n);
    }
    else if(var == "omega")
    {
        mPsOmega = value.toDouble();
    }
    else if(var == "c1")
    {
        mPsC1 = value.toDouble();
    }
    else if(var == "c2")
    {
        mPsC2 = value.toDouble();
    }
}

double OptimizationWorkerParticleSwarm::getOptVar(const QString &var, bool &ok)
{
    double retval = OptimizationWorker::getOptVar(var, ok);
    if(ok)
    {
        return retval;
    }

    ok = true;
    if(var == "omega")
    {
        return mPsOmega;
    }
    else if(var == "c1")
    {
        return mPsC1;
    }
    else if(var == "c2")
    {
        return mPsC2;
    }
    else
    {
        ok = false;
        return 0;
    }
}
