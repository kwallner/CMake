/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExtraVSCodeGenerator_h
#define cmExtraVSCodeGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

#include "cmExternalMakefileProjectGenerator.h"

class cmGeneratedFileStream;
class cmLocalGenerator;

/** \class cmExtraVSCodeGenerator
 * \brief Write Kate project files for Makefile or ninja based projects
 */
class cmExtraVSCodeGenerator : public cmExternalMakefileProjectGenerator
{
public:
  cmExtraVSCodeGenerator();

  static cmExternalMakefileProjectGeneratorFactory* GetFactory();

  void Generate() override;
};

#endif
