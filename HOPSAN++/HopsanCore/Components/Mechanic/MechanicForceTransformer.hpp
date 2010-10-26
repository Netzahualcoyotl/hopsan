//$Id$

#ifndef MECHANICFORCETRANSFORMER_HPP_INCLUDED
#define MECHANICFORCETRANSFORMER_HPP_INCLUDED

#include "../../ComponentEssentials.h"

namespace hopsan {

    //!
    //! @brief
    //! @ingroup MechanicalComponents
    //!
    class MechanicForceTransformer : public ComponentC
    {

    private:
        double mStartPosition;
        double mStartVelocity;
        double mStartForce;
        double mSignal;
        Port *mpIn, *mpP1;

    public:
        static Component *Creator()
        {
            return new MechanicForceTransformer("ForceTransformer");
        }

        MechanicForceTransformer(const std::string name) : ComponentC(name)
        {
            //Set member attributes
            mTypeName = "MechanicForceTransformer";
            mStartPosition = 0.0;
            mStartVelocity = 0.0;
            mStartForce = 0.0;
            mSignal = 0.0;

            //Add ports to the component
            mpIn = addReadPort("in", "NodeSignal", Port::NOTREQUIRED);
            mpP1 = addPowerPort("P1", "NodeMechanic");

            //Register changable parameters to the HOPSAN++ core
            registerParameter("x0", "Initial Position", "[m]", mStartPosition);
            registerParameter("v0", "Initial Velocity", "[m/s]", mStartVelocity);
            registerParameter("Force", "Generated force", "[N]", mSignal);
        }


        void initialize()
        {
            mpP1->writeNode(NodeMechanic::POSITION, mStartPosition);
            mpP1->writeNode(NodeMechanic::VELOCITY, mStartVelocity);
            mpP1->writeNode(NodeMechanic::FORCE, mStartForce);
            mpP1->writeNode(NodeMechanic::CHARIMP, 0.0);
            mpP1->writeNode(NodeMechanic::WAVEVARIABLE, 0.0);
        }


        void simulateOneTimestep()
        {
            double signal;
            //Get variable values from nodes
            if(mpIn->isConnected())
                signal  = mpIn->readNode(NodeSignal::VALUE);
            else
                signal = mSignal;

            //Transformer equations
            double c = signal;
            double Zc = 0.0;

            //Write new values to nodes
            mpP1->writeNode(NodeMechanic::WAVEVARIABLE, c);
            mpP1->writeNode(NodeMechanic::CHARIMP, Zc);
        }
    };


    //!
    //! @brief
    //! @ingroup MechanicalComponents
    //!
    class MechanicOptimizedForceTransformer : public ComponentC
    {

    private:
        double mStartPosition;
        double mStartVelocity;
        double mStartForce;
        double mSignal;
        Port *mpIn, *mpP1;

        double *x1, *v1, *F1, *c1, *Zc1, *input;

    public:
        static Component *Creator()
        {
            return new MechanicOptimizedForceTransformer("ForceTransformer");
        }

        MechanicOptimizedForceTransformer(const std::string name) : ComponentC(name)
        {
            //Set member attributes
            mTypeName = "MechanicOptimizedForceTransformer";
            mStartPosition = 0.0;
            mStartVelocity = 0.0;
            mStartForce = 0.0;
            mSignal = 0.0;

            //Add ports to the component
            mpIn = addReadPort("in", "NodeSignal", Port::NOTREQUIRED);
            mpP1 = addPowerPort("P1", "NodeMechanic");

            //Register changable parameters to the HOPSAN++ core
            registerParameter("x0", "Initial Position", "[m]", mStartPosition);
            registerParameter("v0", "Initial Velocity", "[m/s]", mStartVelocity);
            registerParameter("Force", "Generated force", "[N]", mSignal);
        }


        void initialize()
        {
            x1 = mpP1->getNodeDataPtr(NodeMechanic::POSITION);
            v1 = mpP1->getNodeDataPtr(NodeMechanic::VELOCITY);
            F1 = mpP1->getNodeDataPtr(NodeMechanic::FORCE);
            c1 = mpP1->getNodeDataPtr(NodeMechanic::WAVEVARIABLE);
            Zc1 = mpP1->getNodeDataPtr(NodeMechanic::CHARIMP);

            if(mpIn->isConnected())
            {
                input = mpIn->getNodeDataPtr(NodeSignal::VALUE);
            }

            *x1 = mStartPosition;
            *v1 = mStartVelocity;
            *F1 = mStartForce;
            *c1 = 0.0;
            *Zc1 = 0.0;
        }


        void simulateOneTimestep()
        {
            if(mpIn->isConnected())
                *c1  = *input;
            else
                *c1 = mSignal;

            *Zc1 = 0.0;
        }
    };
}

#endif // MECHANICFORCETRANSFORMER_HPP_INCLUDED
