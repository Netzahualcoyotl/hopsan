/*-----------------------------------------------------------------------------
 This source file is a part of Hopsan

 Copyright (c) 2009 to present year, Hopsan Group

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 For license details and information about the Hopsan Group see the files
 GPLv3 and HOPSANGROUP in the Hopsan source code root directory

 For author and contributor information see the AUTHORS file
-----------------------------------------------------------------------------*/

//!
//! @file   SimulationHandler.cc
//! @author FluMeS
//! @date   2009-12-20
//!
//! @brief Contains the simulation handler help class
//!
//$Id$

#include "CoreUtilities/SimulationHandler.h"
#include "CoreUtilities/MultiThreadingUtilities.h"
#include "ComponentSystem.h"

using namespace hopsan;
using namespace std;

bool SimulationHandler::initializeSystem(const double startT, const double stopT, ComponentSystem* pSystem)
{
    if (pSystem->checkModelBeforeSimulation())
    {
        return pSystem->initialize(startT, stopT);
    }
    return false;
}

bool SimulationHandler::initializeSystem(const double startT, const double stopT, std::vector<ComponentSystem*> &rSystemVector)
{
    //No multicore init support
    bool isOk = true;
    for (size_t i=0; i<rSystemVector.size(); ++i)
    {
        isOk = isOk && initializeSystem(startT, stopT, rSystemVector[i]);
        if (!isOk)
        {
            break;
        }
    }
    return isOk;
}

bool SimulationHandler::simulateSystem(const double startT, const double stopT, const int nDesiredThreads, ComponentSystem* pSystem, bool noChanges, ParallelAlgorithmT algorithm)
{
    if (nDesiredThreads < 0)
    {
        pSystem->addInfoMessage("Using single-threaded algorithm.");
        pSystem->simulate(stopT);
    }
    else
    {
        pSystem->simulateMultiThreaded(startT, stopT, nDesiredThreads, noChanges, algorithm);
    }

    return !pSystem->wasSimulationAborted();
}

bool SimulationHandler::simulateSystem(const double startT, const double stopT, const int nDesiredThreads, std::vector<ComponentSystem*> &rSystemVector, bool noChanges, ParallelAlgorithmT algorithm)
{
    if (rSystemVector.size() > 1)
    {
        if (nDesiredThreads >= 0)
        {
            return simulateMultipleSystemsMultiThreaded(startT, stopT, nDesiredThreads, rSystemVector, noChanges);
        }
        else
        {
            return simulateMultipleSystems(stopT, rSystemVector);
        }
    }
    else if (rSystemVector.size() == 1)
    {
        return simulateSystem(startT, stopT, nDesiredThreads, rSystemVector[0], noChanges, algorithm);
    }

    return false;
}

void SimulationHandler::finalizeSystem(ComponentSystem* pSystem)
{
    pSystem->finalize();
}

void SimulationHandler::finalizeSystem(std::vector<ComponentSystem*> &rSystemVector)
{
    //No multicore finalize
    for (size_t i=0; i<rSystemVector.size(); ++i)
    {
        finalizeSystem(rSystemVector[i]);
    }
}

void SimulationHandler::runCoSimulation(ComponentSystem *pSystem)
{
    (void)pSystem;
#ifdef USEBOOST

    std::vector<HString> inputComponents;
    std::vector<HString> inputPorts;
    std::vector<int> inputData;

    std::vector<HString> outputComponents;
    std::vector<HString> outputPorts;
    std::vector<int> outputData;

    //////////////////////////

    std::vector<HString> names = pSystem->getSubComponentNames();
    for(size_t i=0; i<names.size(); ++i)
    {
        Component *pComponent = pSystem->getSubComponent(names[i]);
        if(pComponent->getTypeName() == "SignalInputInterface")
        {
            inputComponents.push_back(names[i]);
            inputPorts.push_back("out");
            inputData.push_back(0);
        }
        else if(pComponent->getTypeName() == "SignalOutputInterface")
        {
            outputComponents.push_back(names[i]);
            outputPorts.push_back("in");
            outputData.push_back(0);
        }
    }

    //////////////////////////

    std::vector<double*> inputSockets;
    std::vector<double*> outputSockets;

    //Initialize shared memory sockets

    //Simulate
    double *sim_socket = getDoubleSharedMemoryPointer("hopsan_sim");

    //Stop
    bool *stop_socket = getBoolSharedMemoryPointer("hopsan_stop");


    //Input
    for(int i=0; i<inputData.size(); ++i)
    {
        std::stringstream ss;
        ss << "hopsan_in" << i;
        inputSockets.push_back(getDoubleSharedMemoryPointer(ss.str().c_str()));
    }


    //Output
    for(int i=0; i<inputData.size(); ++i)
    {
        std::stringstream ss;
        ss << "hopsan_out" << i;
        outputSockets.push_back(getDoubleSharedMemoryPointer(ss.str().c_str()));
    }

    //Initialize simulation
    //! @todo We must be able to log data without knowing the stop time
    pSystem->initialize(0, 100);

    (*stop_socket) = false;
    (*sim_socket) = 0;

    (*inputSockets[0]) = 34.125;

    while(!(*stop_socket))
    {
        if((*sim_socket)>5)
        {
            //Set input variables
            for(int i=0; i<inputSockets.size(); ++i)
            {
                pSystem->getSubComponent(inputComponents[i])->getPort(inputPorts[i])->writeNode(inputData[i], (*inputSockets[i]));
            }

            //Simulate one step
            double time = pSystem->getTime();
            double timestep = pSystem->getDesiredTimeStep();
            pSystem->simulate(time, time+timestep);

            //Write back output variables
            for(int i=0; i<outputSockets.size(); ++i)
            {
                (*outputSockets[i]) = pSystem->getSubComponent(outputComponents[i])->getPort(outputPorts[i])->readNode(outputData[i]);
            }

            //Reset simulation flag
            (*sim_socket) = 0;
        }
    }

#endif
}

//! @brief Distributes component system pointers evenly over one vector per thread, depending on their simulation time
//! @param[in] rSystemVector Vector to distribute
//! @param[in] nThreads Number of threads to distribute for
vector< vector<ComponentSystem *> > SimulationHandler::distributeSystems(const std::vector<ComponentSystem *> &rSystemVector, size_t nThreads)
{
    vector< vector<ComponentSystem *> > splitSystemVector;
    vector<double> timeVector;

    nThreads = min(nThreads, rSystemVector.size()); //Prevent adding for more threads then systems
    splitSystemVector.resize(nThreads);
    timeVector.resize(nThreads,0);
    size_t sysNum=0;
    while(true)         //! @todo Poor algorithm for distributing, will not give optimal results
    {
        for(size_t t=0; t<nThreads; ++t)
        {
            if(sysNum == rSystemVector.size())
                break;
            splitSystemVector[t].push_back(rSystemVector[sysNum]);
            timeVector[t] += rSystemVector[sysNum]->getMeasuredTime();
            ++sysNum;
        }
        if(sysNum == rSystemVector.size())
            break;
    }
    return splitSystemVector;
}

//! @brief Simulates several systems sequentially until given stop time
//! @param[in] stopT Stop time for all systems
//! @param[in] rSystemVector Vector of pointers to component systems to simulate
//! @returns true if successful else false if simulation was aborted for some reason
bool SimulationHandler::simulateMultipleSystems(const double stopT, vector<ComponentSystem*> &rSystemVector)
{
    bool aborted = false;
    for(size_t i=0; i<rSystemVector.size(); ++i)
    {
        rSystemVector[i]->simulate(stopT);
        aborted = aborted && rSystemVector[i]->wasSimulationAborted(); //!< @todo this will give abort=true if one the systems fail, maybe we should abort entirely when one do
    }
    return !aborted;
}

//! @brief Simulates a vector of component systems in parallel, by assigning one or more system to each simulation thread
//! @param startT Start time for all systems
//! @param stopT Stop time for all systems
//! @param nDesiredThreads Desired number of threads (may change due to hardware limitations)
//! @param rSystemVector Vector of pointers to systems to simulate
bool SimulationHandler::simulateMultipleSystemsMultiThreaded(const double startT, const double stopT, const size_t nDesiredThreads, vector<ComponentSystem*> &rSystemVector, bool noChanges)
{
    HOPSAN_UNUSED(startT)
    HOPSAN_UNUSED(nDesiredThreads)
    HOPSAN_UNUSED(noChanges)
#ifdef USETBB
    size_t nThreads = determineActualNumberOfThreads(nDesiredThreads);              //Calculate how many threads to actually use

//    for(size_t i=0; i<rSystemVector.size(); ++i)                         //Loop through the systems, set start time, log nodes and measure simulation time
//    {
//        double *pTime = rSystemVector.at(i)->getTimePtr();
//        *pTime = startT;
////        rSystemVector.at(i)->logTimeAndNodes(*pTime);                        //Log the first time step
//        rSystemVector.at(i)->logTimeAndNodes(0);                        //Log the first time step
//    }

    if(!noChanges)
    {
        mSplitSystemVector.clear();
        for(size_t i=0; i<rSystemVector.size(); ++i)                     //Loop through the systems, set start time, log nodes and measure simulation time
        {
            rSystemVector.at(i)->simulateAndMeasureTime(5);              //Measure time
            rSystemVector.at(i)->initialize(startT, stopT);
        }
        sortSystemsByTotalMeasuredTime(rSystemVector);                   //Sort systems by total measured time
        mSplitSystemVector = distributeSystems(rSystemVector, nThreads); //Distribute systems evenly over split vectors
    }

    tbb::task_group *simTasks;                                          //Initialize TBB routines for parallel simulation
    simTasks = new tbb::task_group;
    for(size_t t=0; t < min(nThreads,mSplitSystemVector.size()); ++t)                                  //Execute simulation
    {
        simTasks->run(taskSimWholeSystems(mSplitSystemVector[t], stopT));
    }
    simTasks->wait();                                                   //Wait for all tasks to finish
    delete(simTasks);

    bool aborted=false;
    for(size_t i=0; i<rSystemVector.size(); ++i)
    {
        aborted = aborted && rSystemVector[i]->wasSimulationAborted();
    }
    return !aborted;
#else
    // Use single core simulation if no TBB support
    return simulateMultipleSystems(stopT, rSystemVector);
#endif
}

//! @brief Sorts a vector of component system pointers by their required simulation time
//! @param [in out]systemVector Vector with system pointers to sort
void SimulationHandler::sortSystemsByTotalMeasuredTime(std::vector<ComponentSystem*> &rSystemVector)
{
    size_t i, j;
    ComponentSystem *tempSystem;
    for(i = 1; i < rSystemVector.size(); ++i)
    {
        bool didSwap = false;
        for (j=0; j < (rSystemVector.size()-1); ++j)
        {
            if (rSystemVector[j+1]->getTotalMeasuredTime() > rSystemVector[j]->getTotalMeasuredTime())
            {
                tempSystem = rSystemVector[j];             //Swap elements
                rSystemVector[j] = rSystemVector[j+1];
                rSystemVector[j+1] = tempSystem;
                didSwap = true;               //Indicates that a swap occurred
            }
        }

        if(!didSwap)
        {
            break;
        }
    }
}
