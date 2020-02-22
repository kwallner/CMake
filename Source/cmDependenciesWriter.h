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
  bool ItemExcluded(cmLinkItem const& item);
  bool TargetTypeEnabled(cmStateEnums::TargetType targetType) const;
  std::string ItemNameWithAliases(std::string const& itemName) const;
  const char* DependencyTypeAsString(DependencyType dt);


  std::string FileName;

  int IndentLength;
  bool IndentUseSpaces;

  Json::Value JsonRoot;

  cmGlobalGenerator const* GlobalGenerator;


};

#endif
