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

private:
  void CreateKateProjectFile(const cmLocalGenerator& lg) const;
  void CreateDummyKateProjectFile(const cmLocalGenerator& lg) const;
  void WriteTargets(const cmLocalGenerator& lg,
                    cmGeneratedFileStream& fout) const;
  void AppendTarget(cmGeneratedFileStream& fout, const std::string& target,
                    const std::string& make, const std::string& makeArgs,
                    const std::string& path,
                    const std::string& homeOutputDir) const;

  std::string GenerateFilesString(const cmLocalGenerator& lg) const;
  std::string GetPathBasename(const std::string& path) const;
  std::string GenerateProjectName(const std::string& name,
                                  const std::string& type,
                                  const std::string& path) const;

  std::string ProjectName;
  bool UseNinja;
};

#endif
