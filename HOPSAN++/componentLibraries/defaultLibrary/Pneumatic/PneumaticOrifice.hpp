#ifndef PNEUMATICORIFICE_HPP_INCLUDED
#define PNEUMATICORIFICE_HPP_INCLUDED

#include <iostream>
#include "ComponentEssentials.h"
#include "ComponentUtilities.h"
#include "math.h"

//!
//! @file PneumaticOrifice.hpp
//! @author Petter Krus <petter.krus@liu.se>
//! @date Mon 11 Mar 2013 17:47:29
//! @brief Pneumatic orifice
//! @ingroup PneumaticComponents
//!
//This code has been autogenerate using Compgen for Hopsan simulation
//from 
/*{, C:, Users, petkr14.IEI, Documents, CompgenNG}/PneumaticNG4.nb*/

using namespace hopsan;

class PneumaticOrifice : public ComponentQ
{
private:
     double mCd;
     double mR;
     double mcv;
     double meps;
     double mA0;
     Port *mpPp1;
     Port *mpPp2;
     Port *mpPA0;
     double delayParts1[9];
     double delayParts2[9];
     double delayParts3[9];
     double delayParts4[9];
     double delayParts5[9];
     Matrix jacobianMatrix;
     Vec systemEquations;
     Matrix delayedPart;
     int i;
     int iter;
     int mNoiter;
     double jsyseqnweight[4];
     int order[5];
     int mNstep;
     //Port Pp1 variable
     double pp1;
     double qmp1;
     double Tp1;
     double dEp1;
     double cp1;
     double Zcp1;
     //Port Pp2 variable
     double pp2;
     double qmp2;
     double Tp2;
     double dEp2;
     double cp2;
     double Zcp2;
//This code has been autogenerate using Compgen for Hopsan simulation
     //inputVariables
     double A0;
     //outputVariables

     //LocalExpressions variables
     double kappa;
     double keps;
     double Kg;
     double Ndenom;
     double crit;
     double cp;
     //Expressions variables
     //Port Pp1 pointer
     double *mpND_pp1;
     double *mpND_qmp1;
     double *mpND_Tp1;
     double *mpND_dEp1;
     double *mpND_cp1;
     double *mpND_Zcp1;
     //Port Pp2 pointer
     double *mpND_pp2;
     double *mpND_qmp2;
     double *mpND_Tp2;
     double *mpND_dEp2;
     double *mpND_cp2;
     double *mpND_Zcp2;
     //Delay declarations
//This code has been autogenerate using Compgen for Hopsan simulation
     //inputVariables pointers
     double *mpND_A0;
     //outputVariables pointers
     Delay mDelayedPart10;
     Delay mDelayedPart20;
     Delay mDelayedPart30;
     EquationSystemSolver *mpSolver;

public:
     static Component *Creator()
     {
        return new PneumaticOrifice();
     }

     void configure()
     {
        const double Cd = 0.65;
        const double R = 287.;
        const double cv = 718;
        const double eps = 0.02;
        const double A0 = 1.e-6;
//This code has been autogenerate using Compgen for Hopsan simulation

        mNstep=9;
        jacobianMatrix.create(5,5);
        systemEquations.create(5);
        delayedPart.create(6,6);
        mNoiter=2;
        jsyseqnweight[0]=1;
        jsyseqnweight[1]=0.67;
        jsyseqnweight[2]=0.5;
        jsyseqnweight[3]=0.5;

        mCd = Cd;
        mR = R;
        mcv = cv;
        meps = eps;
        mA0 = A0;

        //Add ports to the component
        mpPp1=addPowerPort("Pp1","NodePneumatic");
        mpPp2=addPowerPort("Pp2","NodePneumatic");

        //Add inputVariables ports to the component
        mpPA0=addReadPort("PA0","NodeSignal", Port::NOTREQUIRED);

        //Add outputVariables ports to the component

//This code has been autogenerate using Compgen for Hopsan simulation
        //Register changable parameters to the HOPSAN++ core
        registerParameter("Cd", "Discharge coefficient", "", mCd);
        registerParameter("R", "Gas constant", "J/Kg K", mR);
        registerParameter("cv", "heatcoeff", "J/Kg K", mcv);
        registerParameter("eps", "Linearisation coeff", "", meps);
        registerParameter("A0", "Area", "m2", mA0);
        mpSolver = new EquationSystemSolver(this,5);
     }

    void initialize()
     {
        //Read port variable pointers from nodes
        //Port Pp1
        mpND_pp1=getSafeNodeDataPtr(mpPp1, NodePneumatic::PRESSURE);
        mpND_qmp1=getSafeNodeDataPtr(mpPp1, NodePneumatic::MASSFLOW);
        mpND_Tp1=getSafeNodeDataPtr(mpPp1, NodePneumatic::TEMPERATURE);
        mpND_dEp1=getSafeNodeDataPtr(mpPp1, NodePneumatic::ENERGYFLOW);
        mpND_cp1=getSafeNodeDataPtr(mpPp1, NodePneumatic::WAVEVARIABLE);
        mpND_Zcp1=getSafeNodeDataPtr(mpPp1, NodePneumatic::CHARIMP);
        //Port Pp2
        mpND_pp2=getSafeNodeDataPtr(mpPp2, NodePneumatic::PRESSURE);
        mpND_qmp2=getSafeNodeDataPtr(mpPp2, NodePneumatic::MASSFLOW);
        mpND_Tp2=getSafeNodeDataPtr(mpPp2, NodePneumatic::TEMPERATURE);
        mpND_dEp2=getSafeNodeDataPtr(mpPp2, NodePneumatic::ENERGYFLOW);
        mpND_cp2=getSafeNodeDataPtr(mpPp2, NodePneumatic::WAVEVARIABLE);
        mpND_Zcp2=getSafeNodeDataPtr(mpPp2, NodePneumatic::CHARIMP);
        //Read inputVariables pointers from nodes
        mpND_A0=getSafeNodeDataPtr(mpPA0, NodeSignal::VALUE,mA0);
        //Read outputVariable pointers from nodes

        //Read variables from nodes
        //Port Pp1
        pp1 = (*mpND_pp1);
        qmp1 = (*mpND_qmp1);
        Tp1 = (*mpND_Tp1);
        dEp1 = (*mpND_dEp1);
        cp1 = (*mpND_cp1);
        Zcp1 = (*mpND_Zcp1);
        //Port Pp2
        pp2 = (*mpND_pp2);
        qmp2 = (*mpND_qmp2);
        Tp2 = (*mpND_Tp2);
        dEp2 = (*mpND_dEp2);
        cp2 = (*mpND_cp2);
        Zcp2 = (*mpND_Zcp2);

        //Read inputVariables from nodes
        A0 = (*mpND_A0);

        //Read outputVariables from nodes

//This code has been autogenerate using Compgen for Hopsan simulation

        //LocalExpressions
        kappa = 1 + mR/mcv;
        keps = 1/(-Sqrt(meps) + Sqrt(1 + meps));
        Kg = Sqrt((Power(2,(1 + kappa)/(-1 + kappa))*kappa*Power(1/(1 + \
kappa),(1 + kappa)/(-1 + kappa)))/mR);
        Ndenom = Power(2,-1 + (1 + kappa)/(-1 + kappa))*(-1 + \
kappa)*Power(1/(1 + kappa),(1 + kappa)/(-1 + kappa));
        crit = Power(2,kappa/(-1 + kappa))*Power(1/(1 + kappa),kappa/(-1 + \
kappa));
        cp = mcv + mR;

        //Initialize delays

        delayedPart[1][1] = delayParts1[1];
        delayedPart[2][1] = delayParts2[1];
        delayedPart[3][1] = delayParts3[1];
        delayedPart[4][1] = delayParts4[1];
        delayedPart[5][1] = delayParts5[1];
     }
    void simulateOneTimestep()
     {
        Vec stateVar(5);
        Vec stateVark(5);
        Vec deltaStateVar(5);

        //Read variables from nodes
        //Port Pp1
        Tp1 = (*mpND_Tp1);
        cp1 = (*mpND_cp1);
        Zcp1 = (*mpND_Zcp1);
        //Port Pp2
        Tp2 = (*mpND_Tp2);
        cp2 = (*mpND_cp2);
        Zcp2 = (*mpND_Zcp2);

        //Read inputVariables from nodes
        A0 = (*mpND_A0);

        //LocalExpressions
        kappa = 1 + mR/mcv;
        keps = 1/(-Sqrt(meps) + Sqrt(1 + meps));
        Kg = Sqrt((Power(2,(1 + kappa)/(-1 + kappa))*kappa*Power(1/(1 + \
kappa),(1 + kappa)/(-1 + kappa)))/mR);
        Ndenom = Power(2,-1 + (1 + kappa)/(-1 + kappa))*(-1 + \
kappa)*Power(1/(1 + kappa),(1 + kappa)/(-1 + kappa));
        crit = Power(2,kappa/(-1 + kappa))*Power(1/(1 + kappa),kappa/(-1 + \
kappa));
        cp = mcv + mR;

        //Initializing variable vector for Newton-Raphson
        stateVark[0] = qmp2;
        stateVark[1] = dEp1;
        stateVark[2] = dEp2;
        stateVark[3] = pp1;
        stateVark[4] = pp2;

        //Iterative solution using Newton-Rapshson
        for(iter=1;iter<=mNoiter;iter++)
        {
         //Orifice
         //Differential-algebraic system of equation parts

          //Assemble differential-algebraic equations
          systemEquations[0] =qmp2 + (A0*Kg*mCd*onPositive(pp1 - \
pp2)*(pp2*Sqrt(Tp1)*onNegative(pp1 - pp2) - pp1*Sqrt(Tp2)*onPositive(pp1 - \
pp2))*(onNegative(-crit + pp2/pp1) + keps*onPositive(-crit + \
pp2/pp1)*signedSquareL((-Power(pp2/pp1,1 + 1/kappa) + \
Power(pp2/pp1,2/kappa))/Ndenom,meps)))/(Sqrt(Tp1)*Sqrt(Tp2));
          systemEquations[1] =dEp1 + (qmp2*(cp*pp2*Tp1*onNegative(-qmp2) + \
(cp*pp2 + mR*(-pp1 + pp2))*Tp2*onPositive(-qmp2)))/pp2;
          systemEquations[2] =dEp2 - (qmp2*(cp*pp1*Tp2*onNegative(qmp2) + \
(cp*pp1 + mR*(pp1 - pp2))*Tp1*onPositive(qmp2)))/pp1;
          systemEquations[3] =-cp1 + pp1 - dEp1*Zcp1;
          systemEquations[4] =-cp2 + pp2 - dEp2*Zcp2;

          //Jacobian matrix
          jacobianMatrix[0][0] = 1;
          jacobianMatrix[0][1] = 0;
          jacobianMatrix[0][2] = 0;
          jacobianMatrix[0][3] = (A0*Kg*mCd*onPositive(pp1 - \
pp2)*((keps*((-2*pp2*Power(pp2/pp1,-1 + 2/kappa))/(kappa*Power(pp1,2)) + ((1 \
+ \
1/kappa)*pp2*Power(pp2/pp1,1/kappa))/Power(pp1,2))*dxSignedSquareL((-Power(pp\
2/pp1,1 + 1/kappa) + \
Power(pp2/pp1,2/kappa))/Ndenom,meps)*(pp2*Sqrt(Tp1)*onNegative(pp1 - pp2) - \
pp1*Sqrt(Tp2)*onPositive(pp1 - pp2))*onPositive(-crit + pp2/pp1))/Ndenom - \
Sqrt(Tp2)*onPositive(pp1 - pp2)*(onNegative(-crit + pp2/pp1) + \
keps*onPositive(-crit + pp2/pp1)*signedSquareL((-Power(pp2/pp1,1 + 1/kappa) + \
Power(pp2/pp1,2/kappa))/Ndenom,meps))))/(Sqrt(Tp1)*Sqrt(Tp2));
          jacobianMatrix[0][4] = (A0*Kg*mCd*onPositive(pp1 - \
pp2)*((keps*((2*Power(pp2/pp1,-1 + 2/kappa))/(kappa*pp1) - ((1 + \
1/kappa)*Power(pp2/pp1,1/kappa))/pp1)*dxSignedSquareL((-Power(pp2/pp1,1 + \
1/kappa) + Power(pp2/pp1,2/kappa))/Ndenom,meps)*(pp2*Sqrt(Tp1)*onNegative(pp1 \
- pp2) - pp1*Sqrt(Tp2)*onPositive(pp1 - pp2))*onPositive(-crit + \
pp2/pp1))/Ndenom + Sqrt(Tp1)*onNegative(pp1 - pp2)*(onNegative(-crit + \
pp2/pp1) + keps*onPositive(-crit + pp2/pp1)*signedSquareL((-Power(pp2/pp1,1 + \
1/kappa) + Power(pp2/pp1,2/kappa))/Ndenom,meps))))/(Sqrt(Tp1)*Sqrt(Tp2));
          jacobianMatrix[1][0] = (cp*pp2*Tp1*onNegative(-qmp2) + (cp*pp2 + \
mR*(-pp1 + pp2))*Tp2*onPositive(-qmp2))/pp2;
          jacobianMatrix[1][1] = 1;
          jacobianMatrix[1][2] = 0;
          jacobianMatrix[1][3] = -((mR*qmp2*Tp2*onPositive(-qmp2))/pp2);
          jacobianMatrix[1][4] = (qmp2*(cp*Tp1*onNegative(-qmp2) + (cp + \
mR)*Tp2*onPositive(-qmp2)))/pp2 - (qmp2*(cp*pp2*Tp1*onNegative(-qmp2) + \
(cp*pp2 + mR*(-pp1 + pp2))*Tp2*onPositive(-qmp2)))/Power(pp2,2);
          jacobianMatrix[2][0] = (-(cp*pp1*Tp2*onNegative(qmp2)) - (cp*pp1 + \
mR*(pp1 - pp2))*Tp1*onPositive(qmp2))/pp1;
          jacobianMatrix[2][1] = 0;
          jacobianMatrix[2][2] = 1;
          jacobianMatrix[2][3] = -((qmp2*(cp*Tp2*onNegative(qmp2) + (cp + \
mR)*Tp1*onPositive(qmp2)))/pp1) + (qmp2*(cp*pp1*Tp2*onNegative(qmp2) + \
(cp*pp1 + mR*(pp1 - pp2))*Tp1*onPositive(qmp2)))/Power(pp1,2);
          jacobianMatrix[2][4] = (mR*qmp2*Tp1*onPositive(qmp2))/pp1;
          jacobianMatrix[3][0] = 0;
          jacobianMatrix[3][1] = -Zcp1;
          jacobianMatrix[3][2] = 0;
          jacobianMatrix[3][3] = 1;
          jacobianMatrix[3][4] = 0;
          jacobianMatrix[4][0] = 0;
          jacobianMatrix[4][1] = 0;
          jacobianMatrix[4][2] = -Zcp2;
          jacobianMatrix[4][3] = 0;
          jacobianMatrix[4][4] = 1;
//This code has been autogenerate using Compgen for Hopsan simulation

          //Solving equation using LU-faktorisation
          mpSolver->solve(jacobianMatrix, systemEquations, stateVark, iter);
          qmp2=stateVark[0];
          dEp1=stateVark[1];
          dEp2=stateVark[2];
          pp1=stateVark[3];
          pp2=stateVark[4];
          //Expressions
          qmp1 = -qmp2;
        }

        //Calculate the delayed parts

        delayedPart[1][1] = delayParts1[1];
        delayedPart[2][1] = delayParts2[1];
        delayedPart[3][1] = delayParts3[1];
        delayedPart[4][1] = delayParts4[1];
        delayedPart[5][1] = delayParts5[1];

        //Write new values to nodes
        //Port Pp1
        (*mpND_pp1)=pp1;
        (*mpND_qmp1)=qmp1;
        (*mpND_dEp1)=dEp1;
        //Port Pp2
        (*mpND_pp2)=pp2;
        (*mpND_qmp2)=qmp2;
        (*mpND_dEp2)=dEp2;
        //outputVariables

        //Update the delayed variabels

     }
};
#endif // PNEUMATICORIFICE_HPP_INCLUDED