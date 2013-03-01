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
//! @file   MechanicSwashPlate.hpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2012-10-12
//!
//! @brief Contains a mechanic swivel plate component
//!
//$Id$

#ifndef MECHANICSWASHPLATE_HPP_INCLUDED
#define MECHANICSWASHPLATE_HPP_INCLUDED

#include "ComponentEssentials.h"
#include "ComponentUtilities.h"

namespace hopsan {

    //!
    //! @brief
    //! @ingroup MechanicalComponents
    //!
    class MechanicSwashPlate : public ComponentQ
    {

    private:
        Port *mpIn1, *mpIn2, *mpOut1, *mpP1;
        double *mpND_in1, *mpND_in2, *mpND_out1;
        std::vector<double*> mvpND_f1, mvpND_x1, mvpND_v1, mvpND_c1, mvpND_Zc1, mvpND_me1;
        size_t mNumPorts1;

        Integrator mIntegrator;

        double r,offset, startX;

        std::vector<double> f1, c1, Zc1, x1,v1;

    public:
        static Component *Creator()
        {
            return new MechanicSwashPlate();
        }

        void configure()
        {
            r = 0.05;
            offset = 0.0;

            //Register changable parameters to the HOPSAN++ core
            registerParameter("r", "Swivel Radius", "[m]", r);
            registerParameter("theta_offset", "Angle Offset", "[m]", offset);

            //Add ports to the component
            mpIn1 = addReadPort("angle", "NodeSignal");
            mpIn2 = addReadPort("movement", "NodeSignal");
            mpOut1 = addWritePort("torque", "NodeSignal");

            mpP1 = addPowerMultiPort("P1", "NodeMechanic");
        }


        void initialize()
        {
            mNumPorts1 = mpP1->getNumPorts();
            mvpND_f1.resize(mNumPorts1);
            mvpND_x1.resize(mNumPorts1);
            mvpND_v1.resize(mNumPorts1);
            mvpND_c1.resize(mNumPorts1);
            mvpND_Zc1.resize(mNumPorts1);
            mvpND_me1.resize(mNumPorts1);
            f1.resize(mNumPorts1);
            c1.resize(mNumPorts1);
            Zc1.resize(mNumPorts1);
            x1.resize(mNumPorts1);
            v1.resize(mNumPorts1);

            mpND_in1 = getSafeNodeDataPtr(mpIn1, NodeSignal::VALUE);
            mpND_in2 = getSafeNodeDataPtr(mpIn2, NodeSignal::VALUE);
            mpND_out1 = getSafeNodeDataPtr(mpOut1, NodeSignal::VALUE);

            //Assign node data pointers
            for (size_t i=0; i<mNumPorts1; ++i)
            {
                mvpND_f1[i] = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::FORCE, 0.0);
                mvpND_x1[i] = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::POSITION, 0.0);
                mvpND_v1[i] = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::VELOCITY, 0.0);
                mvpND_c1[i] = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::WAVEVARIABLE, 0.0);
                mvpND_Zc1[i] = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::CHARIMP, 0.0);
                mvpND_me1[i] = getSafeMultiPortNodeDataPtr(mpP1, i, NodeMechanic::EQMASS, 0.0);
            }

            mIntegrator.initialize(mTimestep, 0.0, 0.0);

            for (size_t i=0; i<mNumPorts1; ++i)
            {
                (*mvpND_me1[i]) = 0.02;
            }

            startX = (*mvpND_x1[0]);
        }


        void simulateOneTimestep()
        {
            //Get variable values from nodes
            double angle = (*mpND_in1);
            double w1 = (*mpND_in2);

            double s = r*tan(angle);
            double diff = 2*3.1416/mNumPorts1;

            double a1 = mIntegrator.update(w1);

            //Calculate positions and velocities
            for(size_t i=0; i<mNumPorts1; ++i)
            {
                v1[i] = s*cos(a1-offset-diff*i)*w1;
                x1[i] = startX+s*sin(a1-offset-diff*i);
            }

            //Piston forces
            for(size_t i=0; i<mNumPorts1; ++i)
            {
                c1[i] = (*mvpND_c1[i]);
                Zc1[i] = (*mvpND_Zc1[i]);
                f1[i] = c1[i]+Zc1[i]*v1[i];
            }

            //Calculate torque
            double torque = 0;
            for(size_t i=0; i<mNumPorts1; ++i)
            {
                torque += f1[i]*tan(angle)*r*cos(a1-offset-diff*i);
            }

            //Write new values to nodes
            (*mpND_out1) = torque;
            for(size_t i=0; i<mNumPorts1; ++i)
            {
                (*mvpND_f1[i]) = f1[i];
                (*mvpND_x1[i]) = x1[i];
                (*mvpND_v1[i]) = v1[i];
            }
        }
    };
}

#endif // MECHANICSWASHPLATE_HPP_INCLUDED
