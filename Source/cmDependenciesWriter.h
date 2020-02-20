/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef CMDEPENDENCIESWRITER_H
#define CMDEPENDENCIESWRITER_H

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cmsys/RegularExpression.hxx"

#include "cmGeneratedFileStream.h"
#include "cmLinkItemGraphVisitor.h"
#include "cmStateTypes.h"
#include "cm_jsoncpp_value.h"

class cmLinkItem;
class cmGlobalGenerator;

/** This class implements writing files for graphviz (dot) for graphs
 * representing the dependencies between the targets in the project. */
class cmDependenciesWriter : public cmLinkItemGraphVisitor
{
public:
  cmDependenciesWriter(std::string const& fileName,
                   const cmGlobalGenerator* globalGenerator);
  ~cmDependenciesWriter() override;

  void VisitGraph(std::string const& name) override;

  void OnItem(cmLinkItem const& item) override;

  void OnDirectLink(cmLinkItem const& depender, cmLinkItem const& dependee,
                    DependencyType dt) override;

  void OnIndirectLink(cmLinkItem const& depender,
                      cmLinkItem const& dependee) override;

  void ReadSettings(const std::string& settingsFileName,
                    const std::string& fallbackSettingsFileName);

  void Write();

private:
  using FileStreamMap =
    std::map<std::string, std::unique_ptr<cmGeneratedFileStream>>;

  void VisitLink(cmLinkItem const& depender, cmLinkItem const& dependee,
                 bool isDirectLink, std::string const& scopeType = "");

  bool ItemExcluded(cmLinkItem const& item);
  bool ItemNameFilteredOut(std::string const& itemName);
  bool TargetTypeEnabled(cmStateEnums::TargetType targetType) const;

  std::string ItemNameWithAliases(std::string const& itemName) const;

  static std::string EscapeForDotFile(std::string const& str);

  static std::string PathSafeString(std::string const& str);

  std::string FileName;

  int IndentLength;
  bool IndentUseSpaces;

  Json::Value DependenciesRoot;

  std::vector<cmsys::RegularExpression> TargetsToIgnoreRegex;

  cmGlobalGenerator const* GlobalGenerator;

  //int NextNodeId;
  // maps from the actual item names to node names in dot:
  std::map<std::string, std::string> NodeNames;

  bool GenerateForExecutables;
  bool GenerateForStaticLibs;
  bool GenerateForSharedLibs;
  bool GenerateForModuleLibs;
  bool GenerateForInterfaceLibs;
  bool GenerateForObjectLibs;
  bool GenerateForUnknownLibs;
  bool GenerateForCustomTargets;
  bool GenerateForExternals;
  bool GeneratePerTarget;
  bool GenerateDependers;
};

#endif
