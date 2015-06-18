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
// Kurucz Line class (kzline.h)
//==============================================================================
// Creates a spectral line object from a single line in a Kurucz line list. The
// line properties mirror those listed in the Kurucz database. A new line can be
// created by passing a full line string from a Kurucz line list into the class
// constructor, KzLine (std::string), or by using the readLine (std::string)
// function. Individual line properties may be modified or queried by using 
// their respective SET and GET functions. A complete report of the line
// properties can be returned in the Kurucz format with the lineString () 
// function.
//
// If a list of Kurucz lines is required, the KzList class provides
// additional functionality beyond that given by a std::vector.
//
#ifndef KZ_LINE_H
#define KZ_LINE_H

#include <string>
#include <fstream>
#include <cstring>
#include <vector>
#include <stdexcept>
#include "ErrDefs.h"

// Each record of an atomic line in the Kurucz database is of a fixed length.
// Note this length here so it can be used for error checking in readLine ().
#define KZ_RECORD_LENGTH 160 /* characters */

using namespace::std;

class KzLine {

private:

  // Class variables for each of the line properties given by Kurucz
  double Lambda;
  double Sigma; bool SigmaSet;
  double Loggf;
  double Code;
  double ELower;
  double JLower;
  std::string ConfigLower;
  double EUpper;
  double JUpper;
  std::string ConfigUpper;
  double GammaRad;
  double GammaStark;
  double GammaWaals;
  std::string Ref;
  int NlteLower;
  int NlteUpper;
  int Isotope;
  double HfStrength;
  int Isotope2;
  double IsotopeAbundance;
  int HfShiftLower;
  int HfShiftUpper;
  int HfFLower;
  char HfNoteLower;
  int HfFUpper;
  char HfNoteUpper;
  int StrengthClass;
  std::string TagCode;
  int LandeGLower;
  int LandeGUpper;
  int IsotopeShift;
  
  // Additional class variables for calculating line properties derived from
  // the above Kurucz properties.
  double BranchingFraction;
  double TransitionProb;
  double Lifetime;
  double LifetimeError;

public:

  // Constructors and destructors
  KzLine ();
  KzLine (std::string LineInfo);
  ~KzLine () { /* Does nothing */ };
  void initClass ();
  
  // I/O functions for dealing with text records from a Kurucz line list.
  void readLine (std::string LineInfoIn) throw (Error);
  std::string lineString ();
  
  // I/O functions for saving/loading a KzLine from to/from a FAST project file.
  void save (std::ofstream& BinOut);
  void load (std::ifstream& BinIn);
  
  // Public SET functions for Kurucz line properties.
  void lambda (double newLambda) { Lambda = newLambda; }
  void loggf (double newLoggf) { Loggf = newLoggf; }
  void code (double newCode) { Code = newCode; }
  void eLower (double newELower) { ELower = newELower; }
  void jLower (double newJLower) { JLower = newJLower; }
  void configLower (std::string newConfigLower) { ConfigLower = newConfigLower;}
  void eUpper (double newEUpper) { EUpper = newEUpper; }
  void jUpper (double newJUpper) { JUpper = newJUpper; }
  void configUpper (std::string newConfigUpper) { ConfigUpper = newConfigUpper;}
  void gammaRad (double newGammaRad) { GammaRad = newGammaRad; }
  void gammaStark (double newGammaStark) { GammaStark = newGammaStark; }
  void gammaWaals (double newGammaWaals) { GammaWaals = newGammaWaals; }
  void ref (std::string newRef) { Ref = newRef; }
  void nlteLower (int newNlteLower) { NlteLower = newNlteLower; }
  void nlteUpper (int newNlteUpper) { NlteUpper = newNlteUpper; }
  void isotope (int newIsotope) { Isotope = newIsotope; }
  void hfStrength (double newHfStrength) { HfStrength = newHfStrength; }
  void isotope2 (int newIsotope2) { Isotope2 = newIsotope2; }
  void isotopeAbundance (double newIsotopeAbundance) { IsotopeAbundance = newIsotopeAbundance; }
  void hfShiftLower (int newHfShiftLower) { HfShiftLower = newHfShiftLower; }
  void hfShiftUpper (int newHfShiftUpper) { HfShiftUpper = newHfShiftUpper; }
  void hfFLower (int newHfFLower) { HfFLower = newHfFLower; }
  void hfNoteLower (char newHfNoteLower) { HfNoteLower = newHfNoteLower; }
  void hfFUpper (int newHfFUpper) { HfFUpper = newHfFUpper; }
  void hfNoteUpper (char newHfNoteUpper) { HfNoteUpper = newHfNoteUpper; }
  void strengthClass (int newStrengthClass) { StrengthClass = newStrengthClass;}
  void tagCode (std::string newTagCode) { TagCode = newTagCode; }
  void landeGLower (int newLandeGLower) { LandeGLower = newLandeGLower; }
  void landeGUpper (int newLandeGUpper) { LandeGUpper = newLandeGUpper; }
  void isotopeShift (int newIsotopeShift) { IsotopeShift = newIsotopeShift; }

  // Public SET functions for derived line properties.
  void sigma (double newSigma) { Sigma = newSigma; SigmaSet = true; }
  void brFrac (double newBrFrac) { BranchingFraction = newBrFrac; }
  void trProb (double newTrProb) { TransitionProb = newTrProb; }
  void lifetime (double newLifetime) { Lifetime = newLifetime; }
  void lifetime_error (double newErr) { LifetimeError = newErr; }
  
  // Public GET functions for Kurucz line properties.
  double lambda ()            { return Lambda; }
  double loggf ()             { return Loggf; }
  double code ()              { return Code; }
  double eLower ()            { return ELower; }
  double energyLower ()       { return (EUpper < ELower) ? EUpper : ELower; }
  double jLower ()            { return JLower; }
  std::string configLower ()  { return ConfigLower; }
  double eUpper ()            { return EUpper; }
  double energyUpper ()       { return (EUpper > ELower) ? EUpper : ELower; }
  double jUpper ()            { return JUpper; }
  std::string configUpper ()  { return ConfigUpper; }
  double gammaRad ()          { return GammaRad; }
  double gammaStark ()        { return GammaStark; }
  double gammaWaals ()        { return GammaWaals; }
  std::string ref ()          { return Ref; }
  int nlteLower ()            { return NlteLower; }
  int nlteUpper ()            { return NlteUpper; }
  int isotope ()              { return Isotope; }
  double hfStrength ()        { return HfStrength; }
  int isotope2 ()             { return Isotope2; }
  double isotopeAbundance ()  { return IsotopeAbundance; }
  int hfShiftLower ()         { return HfShiftLower; }
  int hfShiftUpper ()         { return HfShiftUpper; }
  int hfFLower ()             { return HfFLower; }
  char hfNoteLower ()         { return HfNoteLower; }
  int hfFUpper ()             { return HfFUpper ; }
  char hfNoteUpper ()         { return HfNoteUpper; }
  int strengthClass ()        { return StrengthClass; }
  std::string tagCode ()      { return TagCode; }
  int landeGLower ()          { return LandeGLower; }
  int landeGUpper ()          { return LandeGUpper; }
  int isotopeShift ()         { return IsotopeShift; }

  // Public GET functions for derived line properties.
  double sigma ();
  double brFrac ()            { return BranchingFraction; }
  double trProb ()            { return TransitionProb; }
  double lifetime ()          { return Lifetime; }
  double lifetime_error ()    { return LifetimeError; }
};

#endif // KZ_LINE_H
