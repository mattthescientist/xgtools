// Xgtools
// Copyright (C) M. P. Ruffoni 2011-2015
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//==============================================================================
// Kurucz Line class (kzline.cpp)
//==============================================================================

#include <sstream>
#include <iostream>
#include <cstdio>
#include <cmath>
#include "kzline.h"

//------------------------------------------------------------------------------
// Default constructor. Initialises all class variables.
//
KzLine::KzLine () {
  initClass ();
}


//------------------------------------------------------------------------------
// String Constructor : The string at arg1 should be a full line of text read
// from a Kurucz line list. This constructor then uses readLine to create a new
// Line object from the information in this string.
//
KzLine::KzLine (std::string LineInfo) {
  initClass ();
  readLine (LineInfo);
}
  
  
//------------------------------------------------------------------------------
// void initClass (): Initialises all class variables.
//
void KzLine::initClass () {
  Lambda = 0.0;
  Sigma = 0.0; SigmaSet = false;
  Loggf = 0.0;
  Code = 0.0;
  ELower = 0.0;
  JLower = 0.0;
  ConfigLower = "          ";
  EUpper = 0.0;
  JUpper = 0.0;
  ConfigUpper = "          ";
  GammaRad = 0.0;
  GammaStark = 0.0;
  GammaWaals = 0.0;
  Ref = "    ";
  NlteLower = 0;
  NlteUpper = 0;
  Isotope = 0;
  HfStrength = 0.0;
  Isotope2 = 0;
  IsotopeAbundance = 0.0;
  HfShiftLower = 0;
  HfShiftUpper = 0;
  HfFLower = 0;
  HfNoteLower = ' ';
  HfFUpper = 0;
  HfNoteUpper = ' ';
  StrengthClass = 0;
  TagCode = "   ";
  LandeGLower = 0;
  LandeGUpper = 0;
  IsotopeShift = 0;
  BranchingFraction = 0.0;
  TransitionProb = 0.0;
  Lifetime = 1.0;
  LifetimeError = 0.0;
}


//------------------------------------------------------------------------------
// readLine (string) : Given a full line of text from a Kurucz line list, this
// function will extract all the line properties and store them in the Line
// object.
//
void KzLine::readLine (string LineInfoIn) throw (Error) {
  istringstream iss;
  string LineInfo = LineInfoIn;

  // First, check that LineInfoIn is of the correct length
  if (LineInfoIn.size () != KZ_RECORD_LENGTH) {
    throw Error (LC_FILE_READ_ERROR);
  }

  // First, explictly read the character fields
  try {
    ConfigLower = LineInfo.substr (42, 10);
    ConfigUpper = LineInfo.substr (70, 10);
    Ref = LineInfo.substr (98, 4);
    HfNoteLower = LineInfo[136];
    HfNoteUpper = LineInfo[139];
    TagCode = LineInfo.substr (141, 3);
  } catch (out_of_range& Err) {
    throw Error (LC_FILE_READ_ERROR);
  }
  
  // Then remove all the character fields to leave only numeric ones
  LineInfo.erase (141, 3);
  LineInfo.erase (LineInfo.begin() + 139);
  LineInfo.erase (LineInfo.begin() + 136);
  LineInfo.erase (LineInfo.begin() + 134);
  LineInfo.erase (98, 4);
  LineInfo.erase (70, 10);
  LineInfo.erase (42, 10);
  
  // Now read the numeric fields from a string stream
  iss.str (LineInfo);
  iss >> Lambda >> Loggf >> Code >> ELower >> JLower >> EUpper >> JUpper
    >> GammaRad >> GammaStark >> GammaWaals >> NlteLower >> NlteUpper
    >> Isotope >> HfStrength >> Isotope2 >> IsotopeAbundance;
  if (!iss.good ()) {
    throw Error (LC_FILE_READ_ERROR);
  }
  
  // Some of the fields are left blank if not used. Attempt to read them one at
  // a time. If any are blank, just skip them and set the property to 0 
  iss.str (LineInfoIn.substr (124, 5)); iss >> HfShiftLower;
  if (iss.fail ()) { HfShiftLower = 0; iss.clear (); }
  iss.str (LineInfoIn.substr (129, 5)); iss >> HfShiftUpper;
  if (iss.fail ()) { HfShiftUpper = 0; iss.clear (); }
  iss.str (LineInfoIn.substr (135, 1)); iss >> HfFLower;
  if (iss.fail ()) { HfFLower = 0; iss.clear (); }
  iss.str (LineInfoIn.substr (138, 1)); iss >> HfFUpper;
  if (iss.fail ()) { HfFUpper = 0; iss.clear (); }
  iss.str (LineInfoIn.substr (140, 1)); iss >> StrengthClass;
  if (iss.fail ()) { StrengthClass = 0; iss.clear (); }
  
  // The next two parameters should always be present
  iss.str (LineInfoIn.substr (144)); 
  iss >> LandeGLower >> LandeGUpper;
  if (iss.fail ()) {
    throw Error (LC_FILE_READ_ERROR);
  }
  
  // The final parameter may or may not be present
  try {
    iss.str (LineInfoIn.substr (154, 6)); iss >> IsotopeShift;
  } catch (out_of_range& Err) {
    IsotopeShift = 0;
  }
}



//------------------------------------------------------------------------------
// lineString () : Compiles all the line properties into a Kurucz line string
// and returns the result.
//
std::string KzLine::lineString () {
  std::string rtnString;
  char str [171];
  sprintf (str, "%11.4f%7.3f%6.2f%12.3f%5.1f %10s%12.3f%5.1f %10s%6.2f%6.2f%6.2f%4s%2i%2i%3i%6.3f%3i%6.3f%5i%5i %i%c %i%c%i%3s%5i%5i%6i",
    Lambda, Loggf, Code, ELower, JLower, ConfigLower.c_str(), EUpper, JUpper, 
    ConfigUpper.c_str(), GammaRad, GammaStark, GammaWaals, Ref.c_str(), NlteLower, 
    NlteUpper, Isotope, HfStrength, Isotope2, IsotopeAbundance, 
    HfShiftLower, HfShiftUpper, HfFLower, HfNoteLower, HfFUpper, 
    HfNoteUpper, StrengthClass, TagCode.c_str(), LandeGLower, LandeGUpper, 
    IsotopeShift);
  
  rtnString = str;
  return rtnString;
}


//------------------------------------------------------------------------------
// save (std::ofstream &) : Saves all the line properties to the binary stream
// specified at arg1.
//
void KzLine::save (std::ofstream &BinOut) {
  int Size;
  
  // Save the properties specified in the Kurucz database, and in the same order
  // in which they are given in a Kurucz line string.
  BinOut.write ((char*) &Lambda, sizeof (double));
  BinOut.write ((char*) &Sigma, sizeof (double)); 
  BinOut.write ((char*) &SigmaSet, sizeof (bool));
  BinOut.write ((char*) &Loggf, sizeof (double));
  BinOut.write ((char*) &Code, sizeof (double));
  BinOut.write ((char*) &ELower, sizeof (double));
  BinOut.write ((char*) &JLower, sizeof (double));
  
  // For string properties the size of the string must be saved as well as its
  // contents
  Size = ConfigLower.size ();
  char ConfLow [Size]; 
  for (int i = 0; i < Size; i ++) {
    ConfLow[i] = ConfigLower[i];
  }
  BinOut.write ((char*) &Size, sizeof (int));
  BinOut.write ((char*) &ConfLow, sizeof (char) * Size);
 
  BinOut.write ((char*) &EUpper, sizeof (double));
  BinOut.write ((char*) &JUpper, sizeof (double));
  
  Size = ConfigUpper.size ();
  char ConfUp [Size]; 
  for (int i = 0; i < Size; i ++) {
    ConfUp[i] = ConfigUpper[i];
  }
  BinOut.write ((char*) &Size, sizeof (int));
  BinOut.write ((char*) &ConfUp, sizeof (char) * Size);  

  BinOut.write ((char*) &GammaRad, sizeof (double));
  BinOut.write ((char*) &GammaStark, sizeof (double));
  BinOut.write ((char*) &GammaWaals, sizeof (double));
  
  Size = Ref.size ();
  char RefChar [Size]; 
  for (int i = 0; i < Size; i ++) {
    RefChar[i] = Ref[i];
  }
  BinOut.write ((char*) &Size, sizeof (int));
  BinOut.write ((char*) &RefChar, sizeof (char) * Size); 
  
  BinOut.write ((char*) &NlteLower, sizeof (int));
  BinOut.write ((char*) &NlteUpper, sizeof (int));
  BinOut.write ((char*) &Isotope, sizeof (int));
  BinOut.write ((char*) &HfStrength, sizeof (double));
  BinOut.write ((char*) &Isotope2, sizeof (int));
  BinOut.write ((char*) &IsotopeAbundance, sizeof (double));
  BinOut.write ((char*) &HfShiftLower, sizeof (int));
  BinOut.write ((char*) &HfShiftUpper, sizeof (int));
  BinOut.write ((char*) &HfFLower, sizeof (int));
  BinOut.write ((char*) &HfNoteLower, sizeof (char));
  BinOut.write ((char*) &HfFUpper, sizeof (int));
  BinOut.write ((char*) &HfNoteUpper, sizeof (char));
  BinOut.write ((char*) &StrengthClass, sizeof (int));

  Size = TagCode.size ();
  char Tag [Size]; 
  for (int i = 0; i < Size; i ++) {
    Tag[i] = TagCode[i];
  }
  BinOut.write ((char*) &Size, sizeof (int));
  BinOut.write ((char*) &Tag, sizeof (char) * Size);    
  
  BinOut.write ((char*) &LandeGLower, sizeof (int));
  BinOut.write ((char*) &LandeGUpper, sizeof (int));
  BinOut.write ((char*) &IsotopeShift, sizeof (int));

  // Finally, save the line properties derived from Kurucz properties
  BinOut.write ((char*) &BranchingFraction, sizeof (double));
  BinOut.write ((char*) &TransitionProb, sizeof (double));
  BinOut.write ((char*) &Lifetime, sizeof (double));
  BinOut.write ((char*) &LifetimeError, sizeof (double));
}


//------------------------------------------------------------------------------
// load (std::ifstream &) : Loads all the line properties from the binary stream
// specified at arg1. It is imperative that the structure of this function
// matches that of the save (std::ofstream) function above or errors will occur
// in loading the data.
//
void KzLine::load (std::ifstream& BinIn) {
  int Size;
  BinIn.read ((char*) &Lambda, sizeof (double));
  BinIn.read ((char*) &Sigma, sizeof (double));
  BinIn.read ((char*) &SigmaSet, sizeof (bool));
  BinIn.read ((char*) &Loggf, sizeof (double));
  BinIn.read ((char*) &Code, sizeof (double));
  BinIn.read ((char*) &ELower, sizeof (double));
  BinIn.read ((char*) &JLower, sizeof (double));
  
  BinIn.read ((char*) &Size, sizeof (int));
  char ConfLow [Size + 1];
  BinIn.read ((char*) &ConfLow, sizeof (char) * Size);
  ConfLow [Size] = '\0';
  ConfigLower = ConfLow;
  
  BinIn.read ((char*) &EUpper, sizeof (double));
  BinIn.read ((char*) &JUpper, sizeof (double));
  
  BinIn.read ((char*) &Size, sizeof (int));
  char ConfUp [Size + 1];
  BinIn.read ((char*) &ConfUp, sizeof (char) * Size);
  ConfUp [Size] = '\0';
  ConfigUpper = ConfUp;
  
  BinIn.read ((char*) &GammaRad, sizeof (double));
  BinIn.read ((char*) &GammaStark, sizeof (double));
  BinIn.read ((char*) &GammaWaals, sizeof (double));
  
  BinIn.read ((char*) &Size, sizeof (int));
  char RefChar [Size + 1];
  BinIn.read ((char*) &RefChar, sizeof (char) * Size);
  RefChar [Size] = '\0';
  Ref = RefChar;
  
  BinIn.read ((char*) &NlteLower, sizeof (int));
  BinIn.read ((char*) &NlteUpper, sizeof (int));
  BinIn.read ((char*) &Isotope, sizeof (int));
  BinIn.read ((char*) &HfStrength, sizeof (double));
  BinIn.read ((char*) &Isotope2, sizeof (int));
  BinIn.read ((char*) &IsotopeAbundance, sizeof (double));
  BinIn.read ((char*) &HfShiftLower, sizeof (int));
  BinIn.read ((char*) &HfShiftUpper, sizeof (int));
  BinIn.read ((char*) &HfFLower, sizeof (int));
  BinIn.read ((char*) &HfNoteLower, sizeof (char));
  BinIn.read ((char*) &HfFUpper, sizeof (int));
  BinIn.read ((char*) &HfNoteUpper, sizeof (char));
  BinIn.read ((char*) &StrengthClass, sizeof (int));

  BinIn.read ((char*) &Size, sizeof (int));
  char Tag [Size + 1];
  BinIn.read ((char*) &Tag, sizeof (char) * Size);
  Tag [Size] = '\0';
  TagCode = Tag;
  
  BinIn.read ((char*) &LandeGLower, sizeof (int));
  BinIn.read ((char*) &LandeGUpper, sizeof (int));
  BinIn.read ((char*) &IsotopeShift, sizeof (int));

  BinIn.read ((char*) &BranchingFraction, sizeof (double));
  BinIn.read ((char*) &TransitionProb, sizeof (double));
  BinIn.read ((char*) &Lifetime, sizeof (double));
  BinIn.read ((char*) &LifetimeError, sizeof (double));
}


//------------------------------------------------------------------------------
// sigma () : Returns the value of Sigma specified by the user in the 
// sigma (double) SET function. If none has been given, this function calculates
// the line wavenumber based on the difference between the line's upper and
// lower energy levels.
//
double KzLine::sigma () {
  if ( SigmaSet ) 
  { 
    return Sigma; 
  } 
  else 
  {
    return std::abs ( EUpper - ELower );
  }
}



