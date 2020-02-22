/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmDependenciesWriter.h"

#include <cctype>
#include <iostream>
#include <memory>
#include <set>
#include <utility>

#include <cm/memory>

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLinkItem.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmStateSnapshot.h"
#include "cmStringAlgorithms.h"
#include "cmSystemTools.h"
#include "cmake.h"

#include "cm_jsoncpp_writer.h"

cmDependenciesWriter::cmDependenciesWriter(std::string const& fileName,
                                   const cmGlobalGenerator* globalGenerator)
  : FileName(fileName)
  , IndentLength(2)
  , IndentUseSpaces(true)
  , DependenciesRoot()
  , GlobalGenerator(globalGenerator)
{
  DependenciesRoot["graph"] = Json::Value();
  DependenciesRoot["graph"]["directed"] = true;
  DependenciesRoot["graph"]["type"] = "graph type";
  DependenciesRoot["graph"]["label"] = GlobalGenerator->GetSafeGlobalSetting("CMAKE_PROJECT_NAME");
  DependenciesRoot["graph"]["metadata"] = Json::Value(Json::objectValue);
  DependenciesRoot["graph"]["nodes"] = Json::Value(Json::objectValue);
  DependenciesRoot["graph"]["edges"] = Json::Value(Json::arrayValue);

  Json::Value& metadata = DependenciesRoot["graph"]["metadata"];
  metadata["project_name"] = globalGenerator->GetSafeGlobalSetting("CMAKE_PROJECT_NAME");

  {
    const auto& lg = GlobalGenerator->GetLocalGenerators()[0];

    const cmMakefile* mf = lg->GetMakefile();
    std::vector<std::string> definitions = mf->GetDefinitions();
    for (const std::string& definiton : definitions) {
      metadata[definiton] = mf->GetSafeDefinition(definiton);
    }
  }

  //metadata["conan_package_name"] = globalGenerator->GetSafeGlobalSetting("CONAN_PACKAGE_NAME");
  //metadata["conan_package_version"] = globalGenerator->GetSafeGlobalSetting("CONAN_PACKAGE_VERSION");
  //metadata["conan_settings_arch"] = globalGenerator->GetSafeGlobalSetting("CONAN_SETTINGS_ARCH");
  //metadata["conan_settings_arch"] = globalGenerator->GetSafeGlobalSetting("CONAN_SETTINGS_BUILD_TYPE");
  //metadata["conan_settings_compiler"] = globalGenerator->GetSafeGlobalSetting("CONAN_SETTINGS_COMPILER");
  //metadata["conan_settings_compiler_runtime"] = globalGenerator->GetSafeGlobalSetting("CONAN_SETTINGS_COMPILER_RUNTIME");
  //metadata["conan_settings_compiler_version"] = globalGenerator->GetSafeGlobalSetting("CONAN_SETTINGS_COMPILER_VERSION");
  //metadata["conan_settings_os"] = globalGenerator->GetSafeGlobalSetting("CONAN_SETTINGS_OS");

  metadata["conan_dependencies"] = Json::Value(Json::arrayValue);
  std::vector<std::string> conan_dependencies = cmSystemTools::SplitString(globalGenerator->GetSafeGlobalSetting("CONAN_DEPENDENCIES"), ';', false);
  for (const auto& conan_dependency : conan_dependencies) 
  {
    metadata["conan_dependencies"].append(conan_dependency);
  }
  // Get 
}

cmDependenciesWriter::~cmDependenciesWriter()
{
  std::ofstream output_file;
  output_file.open(FileName);

  if (output_file.is_open())
  {
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None"; // FIXME: Use config
    builder["indentation"] = "   ";

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(DependenciesRoot, &output_file);
    output_file.close();
  }
  else 
  {
    cmSystemTools::Error("Problem writing Dependencies to file: " + FileName);
  }
}

void cmDependenciesWriter::VisitGraph(std::string const&)
{
}

void cmDependenciesWriter::OnItem(cmLinkItem const& item)
{
  if (this->ItemExcluded(item)) {
    return;
  }

  Json::Value nodeValue(Json::objectValue); 
  nodeValue["type"] = "node type";
  nodeValue["label"] = cmStrCat("Target ", item.AsStr());
  nodeValue["metadata"] = Json::Value(Json::objectValue);
  nodeValue["metadata"]["target_type"] = cmState::GetTargetTypeName(item.Target ? item.Target->GetType() : cmStateEnums::UNKNOWN_LIBRARY);

  DependenciesRoot["graph"]["nodes"][item.AsStr()] = nodeValue;


}

void cmDependenciesWriter::OnDirectLink(cmLinkItem const& depender,
                                    cmLinkItem const& dependee,
                                    DependencyType dt)
{
  // TODO: dt can be DependencyType::LinkPrivate or DependencyType::LinkInterface  
 
  if (this->ItemExcluded(depender) || this->ItemExcluded(dependee)) {
    return;
  }

  Json::Value edgeValue(Json::objectValue);
  edgeValue["relation"] = "edge relationship",
  edgeValue["source"] = depender.AsStr();
  edgeValue["target"] = dependee.AsStr();

  DependenciesRoot["graph"]["edges"].append(edgeValue);
}

void cmDependenciesWriter::OnIndirectLink(cmLinkItem const& depender,
                                      cmLinkItem const& dependee)
{
  // Not interested in indirekt links
  //this->VisitLink(depender, dependee, dt,false);
}


void cmDependenciesWriter::ReadSettings(
  const std::string& settingsFileName,
  const std::string& fallbackSettingsFileName)
{
  cmake cm(cmake::RoleScript, cmState::Unknown);
  cm.SetHomeDirectory("");
  cm.SetHomeOutputDirectory("");
  cm.GetCurrentSnapshot().SetDefaultDefinitions();
  cmGlobalGenerator ggi(&cm);
  cmMakefile mf(&ggi, cm.GetCurrentSnapshot());
  std::unique_ptr<cmLocalGenerator> lg(ggi.CreateLocalGenerator(&mf));

  std::string inFileName = settingsFileName;
  if (!cmSystemTools::FileExists(inFileName)) {
    inFileName = fallbackSettingsFileName;
    if (!cmSystemTools::FileExists(inFileName)) {
      return;
    }
  }

  if (!mf.ReadListFile(inFileName)) {
    cmSystemTools::Error("Problem opening Dependencies options file: " +
      inFileName);
    return;
  }

  std::cout << "Reading GraphViz options file: " << inFileName << std::endl;

#define __set_int_if_set(var, cmakeDefinition)                                \
  do {                                                                        \
    const char* value = mf.GetDefinition(cmakeDefinition);                    \
    if (value) {                                                              \
      (var) = atoi(value);                                                    \
    }                                                                         \
  } while (false)

  __set_int_if_set(this->IndentLength, "DEPENDENCIES_INDENT_LENGTH");

#define __set_bool_if_set(var, cmakeDefinition)                               \
  do {                                                                        \
    const char* value = mf.GetDefinition(cmakeDefinition);                    \
    if (value) {                                                              \
      (var) = mf.IsOn(cmakeDefinition);                                       \
    }                                                                         \
  } while (false)

  __set_bool_if_set(this->IndentUseSpaces, "DEPENDENCIES_INDENT_USE_SPACES");
}

void cmDependenciesWriter::Write()
{
  auto gg = this->GlobalGenerator;

  // We want to traverse in a determined order, such that the output is always
  // the same for a given project (this makes tests reproducible, etc.)
  std::set<cmGeneratorTarget const*, cmGeneratorTarget::StrictTargetComparison>
    sortedGeneratorTargets;

  for (const auto& lg : gg->GetLocalGenerators()) {
    for (const auto& gt : lg->GetGeneratorTargets()) {
      // Reserved targets have inconsistent names across platforms (e.g. 'all'
      // vs. 'ALL_BUILD'), which can disrupt the traversal ordering.
      // We don't need or want them anyway.
      if (!cmGlobalGenerator::IsReservedTarget(gt->GetName())) {
        sortedGeneratorTargets.insert(gt.get());
      }
    }
  }

  for (auto const gt : sortedGeneratorTargets) {
    auto item = cmLinkItem(gt, false, gt->GetBacktrace());
    this->VisitItem(item);
  }
}


bool cmDependenciesWriter::ItemExcluded(cmLinkItem const& item)
{
  auto const itemName = item.AsStr();

  if (this->ItemNameFilteredOut(itemName)) {
    return true;
  }

  if (item.Target == nullptr) {
    return false; // true; // false; // !this->GenerateForExternals;
  }

  if (item.Target->GetType() == cmStateEnums::UTILITY) {
    if ((itemName.find("Nightly") == 0) ||
      (itemName.find("Continuous") == 0) ||
      (itemName.find("Experimental") == 0)) {
      return true;
    }
  }

  if (item.Target->IsImported() /* && !this->GenerateForExternals */ ) {
    return true;
  }

  return !this->TargetTypeEnabled(item.Target->GetType());
}

bool cmDependenciesWriter::ItemNameFilteredOut(std::string const& itemName)
{
  //if (itemName == ">") {
    // FIXME: why do we even receive such a target here?
  //  return true;
  //}

  if (cmGlobalGenerator::IsReservedTarget(itemName)) {
    return true;
  }

  

  return false;
}

bool cmDependenciesWriter::TargetTypeEnabled(
  cmStateEnums::TargetType targetType) const
{
  switch (targetType) {
  case cmStateEnums::EXECUTABLE:
    return true; // this->GenerateForExecutables;
  case cmStateEnums::STATIC_LIBRARY:
    return true; // this->GenerateForStaticLibs;
  case cmStateEnums::SHARED_LIBRARY:
    return true; // this->GenerateForSharedLibs;
  case cmStateEnums::MODULE_LIBRARY:
    return true; // this->GenerateForModuleLibs;
  case cmStateEnums::INTERFACE_LIBRARY:
    return true; //this->GenerateForInterfaceLibs;
  case cmStateEnums::OBJECT_LIBRARY:
    return true; //this->GenerateForObjectLibs;
  case cmStateEnums::UNKNOWN_LIBRARY:
    return true; //this->GenerateForUnknownLibs;
  case cmStateEnums::UTILITY:
    return false; // this->GenerateForCustomTargets;
  case cmStateEnums::GLOBAL_TARGET:
    // Built-in targets like edit_cache, etc.
    // We don't need/want those in the dot file.
    return false;
  default:
    break;
  }
  return false;
}

std::string cmDependenciesWriter::ItemNameWithAliases(
  std::string const& itemName) const
{
  auto nameWithAliases = itemName;

  for (auto const& lg : this->GlobalGenerator->GetLocalGenerators()) {
    for (auto const& aliasTargets : lg->GetMakefile()->GetAliasTargets()) {
      if (aliasTargets.second == itemName) {
        nameWithAliases += "\\n(" + aliasTargets.first + ")";
      }
    }
  }

  return nameWithAliases;
}

std::string cmDependenciesWriter::EscapeForDotFile(std::string const& str)
{
  return cmSystemTools::EscapeChars(str.data(), "\"");
}

std::string cmDependenciesWriter::PathSafeString(std::string const& str)
{
  std::string pathSafeStr;

  // We'll only keep alphanumerical characters, plus the following ones that
  // are common, and safe on all platforms:
  auto const extra_chars = std::set<char>{ '.', '-', '_' };

  for (char c : str) {
    if (std::isalnum(c) || extra_chars.find(c) != extra_chars.cend()) {
      pathSafeStr += c;
    }
  }

  return pathSafeStr;
}
