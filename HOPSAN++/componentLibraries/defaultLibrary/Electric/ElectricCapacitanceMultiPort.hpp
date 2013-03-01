/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2012 
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
//! @file   ElectricCapacitanceMultiPort.hpp
//! @author Petter Krus 
//! @date   2012-01-30
//! based on ElectricCapacitanceMultiPort.hpp by Björn Eriksson <bjorn.eriksson@liu.se>
//! @brief Contains a Electric Volume Component
//!

#ifndef ELECTRICCAPACITANCEMULTIPORT_HPP_INCLUDED
#define ELECTRICCAPACITANCEMULTIPORT_HPP_INCLUDED

#include "ComponentEssentials.h"
#include <vector>
#include <sstream>
#include <iostream>

namespace hopsan {

    //!
    //! @brief A Electric volume component
    //! @ingroup ElectricComponents
    //!
    class ElectricCapacitanceMultiPort : public ComponentC
    {

    private:
        double mZc;
        double mAlpha;
        double mCapacitance;

        std::vector<double*> mvpN_uel, mvpN_iel, mvpN_cel, mvpN_Zcel;
        std::vector<double> mvp_C0;
        size_t mNumPorts;
        Port *mpPel1;

    public:
        static Component *Creator()
        {
            return new ElectricCapacitanceMultiPort();
        }

        void configure()
        {
            //Set member attributes
            mCapacitance   = 0.0001;
            mAlpha         = 0.3;

            //Add ports to the component
            mpPel1 = addPowerMultiPort("Pel1", "NodeElectric");

            //Register changable parameters to the HOPSAN++ core
            registerParameter("Capacitance", "Capacitance", "[Fa]", mCapacitance);
            registerParameter("alpha", "Low pass coeficient to dampen standing delayline waves", "[-]",  mAlpha);

            setStartValue(mpPel1, NodeElectric::CURRENT, 0.0);
            setStartValue(mpPel1, NodeElectric::VOLTAGE, 12);
        }


        void initialize()
        {
            mNumPorts = mpPel1->getNumPorts();

            mvpN_uel.resize(mNumPorts);
            mvpN_iel.resize(mNumPorts);
            mvpN_cel.resize(mNumPorts);
            mvpN_Zcel.resize(mNumPorts);
            mvp_C0.resize(mNumPorts);

            mZc = mNumPorts*mTimestep/(2.0*mCapacitance)/(1.0-mAlpha);

            for (size_t i=0; i<mNumPorts; ++i)
            {
                mvpN_uel[i]  = getSafeMultiPortNodeDataPtr(mpPel1, i, NodeElectric::VOLTAGE, 0.0);
                mvpN_iel[i]  = getSafeMultiPortNodeDataPtr(mpPel1, i, NodeElectric::CURRENT, 0.0);
                mvpN_cel[i]  = getSafeMultiPortNodeDataPtr(mpPel1, i, NodeElectric::WAVEVARIABLE, 0.0);
                mvpN_Zcel[i] = getSafeMultiPortNodeDataPtr(mpPel1, i, NodeElectric::CHARIMP, 0.0);

                *mvpN_uel[i] = getStartValue(mpPel1, NodeElectric::VOLTAGE);
                *mvpN_iel[i] = getStartValue(mpPel1, NodeElectric::CURRENT)/mNumPorts;
                *mvpN_cel[i] = getStartValue(mpPel1, NodeElectric::VOLTAGE);
                *mvpN_Zcel[i] = mZc;
            }
        }


        void simulateOneTimestep()
        {
            double cTot = 0.0;
            double uAvg;

            for (size_t i=0; i<mNumPorts; ++i)
            {
                cTot += (*mvpN_cel[i]) + 2.0*mZc*(*mvpN_iel[i]);
            }
            uAvg = cTot/double(mNumPorts);

            for (size_t i=0; i<mNumPorts; ++i)
            {
                mvp_C0[i] = uAvg*2.0-(*mvpN_cel[i]) - 2.0*mZc*(*mvpN_iel[i]);
                (*mvpN_cel[i]) = mAlpha*(*mvpN_cel[i]) + (1.0-mAlpha)*mvp_C0[i];
                (*mvpN_Zcel[i]) = mZc;
            }
        }


        void finalize()
        {
        }
    };
}

#endif // ELECTRICCAPCITANCEMULTIPORT_HPP_INCLUDED