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
  , JsonRoot()
  , GlobalGenerator(globalGenerator)
{
  JsonRoot["name"] = GlobalGenerator->GetSafeGlobalSetting("CMAKE_PROJECT_NAME");
  JsonRoot["generator"] = GlobalGenerator->GetJson();
  JsonRoot["targets"] = Json::Value(Json::objectValue);
  JsonRoot["links"] = Json::Value(Json::arrayValue);

  {
    const auto& lg = GlobalGenerator->GetLocalGenerators()[0];
    
  }

  {
    const auto& lg = GlobalGenerator->GetLocalGenerators()[0];
    const cmMakefile* mf = lg->GetMakefile();

    Json::Value& cmake_definfions = JsonRoot["definitions"] = Json::Value(Json::objectValue);
    std::vector<std::string> definition_keys = mf->GetDefinitions();
    for (const std::string& definition_key : definition_keys) {
      const std::string& definition_value = mf->GetSafeDefinition(definition_key);
      if (definition_value.find(';') == std::string::npos)
      {
        cmake_definfions[definition_key] = definition_value;
      }
      else 
      {
        std::vector<std::string> definition_list_values = cmSystemTools::SplitString(definition_value, ';');
        Json::Value& cmake_array = cmake_definfions[definition_key] = Json::Value(Json::arrayValue);
        for (const std::string& definition_list_value : definition_list_values) {
          cmake_array.append(definition_list_value);
        }
      }
    }
  }
}

cmDependenciesWriter::~cmDependenciesWriter()
{
  std::ofstream output_file;
  output_file.open(FileName);

  if (output_file.is_open()) {
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None"; // FIXME: Use config?
    builder["indentation"] = "   "; // Fixme: Use ident length and ident use spaces

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(JsonRoot, &output_file);
    output_file.close();
  }
  else {
    cmSystemTools::Error("Problem writing Dependencies to file: " + FileName);
  }
}

void cmDependenciesWriter::VisitGraph(std::string const&)
{
  // CHECK: Not called !!!???
}

void cmDependenciesWriter::OnItem(cmLinkItem const& item)
{
  if (this->ItemExcluded(item)) {
    return;
  }

  Json::Value targetValue(Json::objectValue);
  const std::string& targetName = item.AsStr();
  targetValue["name"] = targetName;

  if (item.Target) {
    const auto& Target = *(item.Target);
    targetValue["type"] = cmState::GetTargetTypeName(Target.GetType());

    {
      Json::Value& targetProperties = targetValue["properties"] = Json::Value(Json::objectValue);

      std::vector<std::string> property_keys = Target.GetPropertyKeys();
      for (const std::string& property_key : property_keys) {
        const std::string& property_value = Target.GetProperty(property_key);
        if (property_value.find(';') == std::string::npos)
        {
          targetProperties[property_key] = property_value;
        }
        else
        {
          std::vector<std::string> property_list_values = cmSystemTools::SplitString(property_value, ';');
          Json::Value& cmake_array = targetValue[property_key] = Json::Value(Json::arrayValue);
          for (const std::string& property_list_value : property_list_values) {
            cmake_array.append(property_list_value);
          }
        }
      }
    }
  }

  JsonRoot["targets"][targetName] = targetValue;
}

void cmDependenciesWriter::OnDirectLink(cmLinkItem const& depender,
                                    cmLinkItem const& dependee,
                                    DependencyType dt)
{
  // TODO: dt can be DependencyType::LinkPrivate or DependencyType::LinkInterface  
 
  if (this->ItemExcluded(depender) || this->ItemExcluded(dependee)) {
    return;
  }

  Json::Value linkValue(Json::objectValue);
  linkValue["source"] = depender.AsStr();
  linkValue["target"] = dependee.AsStr();

  linkValue["dependency_type"] = DependencyTypeAsString(dt);
  
  JsonRoot["links"].append(linkValue);
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

  if (cmGlobalGenerator::IsReservedTarget(itemName)) {
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


const char* cmDependenciesWriter::DependencyTypeAsString(DependencyType dt)
{
  switch (dt)
  {
  case DependencyType::LinkInterface: return "LinkInterface";
  case DependencyType::LinkPublic: return "LinkPublic";
  case DependencyType::LinkPrivate: return "LinkPrivate";
  case DependencyType::Object: return "Object";
  case DependencyType::Utility: return "Utility";
  }

  return "";
 }