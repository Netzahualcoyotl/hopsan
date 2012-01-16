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
//! @file   CSVParser.h
//! @author Björn Eriksson <bjorn.eriksson@liu.se>
//! @date   2011-04-12
//!
//! @brief Contains the Core Utility CSVParser class
//!
//$Id$

#ifndef CVSPARSER_H_INCLUDED
#define CVSPARSER_H_INCLUDED

#include "win32dll.h"
#include <string>
#include <vector>

namespace hopsan {

    //! @ingroup ComponentUtilityClasses
    class DLLIMPORTEXPORT CSVParser
    {
    public:
        CSVParser(bool &success,
                  const std::string filename = "Book1.csv",
                  const char field_terminator = ';',
                  const char line_terminator = '\n',
                  const char enclosure_char = '"',
                  const size_t linesToSkip = 0);

        double interpolate(const double x, const size_t outIndex = 1, const size_t inIndex = 0);

    public: //Should be protected:
        std::vector< std::vector<double> > mData;
    };
}

#endif // CVSPARSER_H_INCLUDED
