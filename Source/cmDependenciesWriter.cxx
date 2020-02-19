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

namespace {

char const* const GRAPHVIZ_EDGE_STYLE_PUBLIC = "solid";
char const* const GRAPHVIZ_EDGE_STYLE_INTERFACE = "dashed";
char const* const GRAPHVIZ_EDGE_STYLE_PRIVATE = "dotted";

char const* const GRAPHVIZ_NODE_SHAPE_EXECUTABLE = "egg"; // egg-xecutable

// Normal libraries.
char const* const GRAPHVIZ_NODE_SHAPE_LIBRARY_STATIC = "octagon";
char const* const GRAPHVIZ_NODE_SHAPE_LIBRARY_SHARED = "doubleoctagon";
char const* const GRAPHVIZ_NODE_SHAPE_LIBRARY_MODULE = "tripleoctagon";

char const* const GRAPHVIZ_NODE_SHAPE_LIBRARY_INTERFACE = "pentagon";
char const* const GRAPHVIZ_NODE_SHAPE_LIBRARY_OBJECT = "hexagon";
char const* const GRAPHVIZ_NODE_SHAPE_LIBRARY_UNKNOWN = "septagon";

char const* const GRAPHVIZ_NODE_SHAPE_UTILITY = "box";

const char* getShapeForTarget(const cmLinkItem& item)
{
  if (item.Target == nullptr) {
    return GRAPHVIZ_NODE_SHAPE_LIBRARY_UNKNOWN;
  }

  switch (item.Target->GetType()) {
    case cmStateEnums::EXECUTABLE:
      return GRAPHVIZ_NODE_SHAPE_EXECUTABLE;
    case cmStateEnums::STATIC_LIBRARY:
      return GRAPHVIZ_NODE_SHAPE_LIBRARY_STATIC;
    case cmStateEnums::SHARED_LIBRARY:
      return GRAPHVIZ_NODE_SHAPE_LIBRARY_SHARED;
    case cmStateEnums::MODULE_LIBRARY:
      return GRAPHVIZ_NODE_SHAPE_LIBRARY_MODULE;
    case cmStateEnums::OBJECT_LIBRARY:
      return GRAPHVIZ_NODE_SHAPE_LIBRARY_OBJECT;
    case cmStateEnums::UTILITY:
      return GRAPHVIZ_NODE_SHAPE_UTILITY;
    case cmStateEnums::INTERFACE_LIBRARY:
      return GRAPHVIZ_NODE_SHAPE_LIBRARY_INTERFACE;
    case cmStateEnums::UNKNOWN_LIBRARY:
    default:
      return GRAPHVIZ_NODE_SHAPE_LIBRARY_UNKNOWN;
  }
}
}

cmDependenciesWriter::cmDependenciesWriter(std::string const& fileName,
                                   const cmGlobalGenerator* globalGenerator)
  : FileName(fileName)
  , IndentLength(2)
  , IndentUseSpaces(true)
  , DependenciesRoot()
  , GlobalGenerator(globalGenerator)
  //, NextNodeId(0)
  , GenerateForExecutables(true)
  , GenerateForStaticLibs(true)
  , GenerateForSharedLibs(true)
  , GenerateForModuleLibs(true)
  , GenerateForInterfaceLibs(true)
  , GenerateForObjectLibs(true)
  , GenerateForUnknownLibs(true)
  , GenerateForCustomTargets(false)
  , GenerateForExternals(true)
  , GeneratePerTarget(true)
  , GenerateDependers(true)
{
  DependenciesRoot["graph"] = Json::Value();
  DependenciesRoot["graph"]["directed"] = true;
  DependenciesRoot["graph"]["type"] = "graph type";
  DependenciesRoot["graph"]["label"] = globalGenerator->GetSafeGlobalSetting("CMAKE_PROJECT_NAME");
  DependenciesRoot["graph"]["metadata"] = Json::Value(Json::objectValue);
  DependenciesRoot["graph"]["nodes"] = Json::Value(Json::objectValue);
  DependenciesRoot["graph"]["edges"] = Json::Value(Json::arrayValue);
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

  Json::Value nodeValue(Json::objectValue); // NextNodeId);
  nodeValue["type"] = "node type";
  nodeValue["label"] = cmStrCat("Target ", item.AsStr());
  nodeValue["metadata"] = Json::Value(Json::objectValue);
  nodeValue["metadata"]["target_type"] = cmState::GetTargetTypeName(item.Target ? item.Target->GetType() : cmStateEnums::UNKNOWN_LIBRARY);

  DependenciesRoot["graph"]["nodes"][item.AsStr()] = nodeValue;

  //DependenciesRoot[item.AsStr()] = Json::Value
  //NodeNames[item.AsStr()] = cmStrCat("", NextNodeId);
  //++NextNodeId;

  //this->WriteNode(this->GlobalFileStream, item);

#if 0
  if (this->GeneratePerTarget) {
    this->CreateTargetFile(this->PerTargetFileStreams, item);
  }

  if (this->GenerateDependers) {
    this->CreateTargetFile(this->TargetDependersFileStreams, item,
                           ".dependers");
  }
#endif
}

#if 0
void cmDependenciesWriter::CreateTargetFile(FileStreamMap& fileStreamMap,
                                        cmLinkItem const& item,
                                        std::string const& fileNameSuffix)
{
  auto const pathSafeItemName = PathSafeString(item.AsStr());
  auto const perTargetFileName =
    cmStrCat(this->FileName, '.', pathSafeItemName, fileNameSuffix);
  auto perTargetFileStream =
    cm::make_unique<cmGeneratedFileStream>(perTargetFileName);

  fileStreamMap.emplace(item.AsStr(), std::move(perTargetFileStream));
}
#endif

void cmDependenciesWriter::OnDirectLink(cmLinkItem const& depender,
                                    cmLinkItem const& dependee,
                                    DependencyType dt)
{
  this->VisitLink(depender, dependee, true, GetEdgeStyle(dt));
}

void cmDependenciesWriter::OnIndirectLink(cmLinkItem const& depender,
                                      cmLinkItem const& dependee)
{
  this->VisitLink(depender, dependee, false);
}

void cmDependenciesWriter::VisitLink(cmLinkItem const& depender,
                                 cmLinkItem const& dependee, bool isDirectLink,
                                 std::string const& scopeType)
{
  if (this->ItemExcluded(depender) || this->ItemExcluded(dependee)) {
    return;
  }

  if (!isDirectLink) {
    return;
  }

  Json::Value edgeValue(Json::objectValue); // NextNodeId);
  edgeValue["relation"] = "edge relationship",
  edgeValue["source"] = dependee.AsStr();
  edgeValue["target"] = depender.AsStr();

  DependenciesRoot["graph"]["edges"].append(edgeValue);

  //this->WriteConnection(this->GlobalFileStream, depender, dependee, scopeType);

#if 0
  if (this->GeneratePerTarget) {
    auto fileStream = PerTargetFileStreams[depender.AsStr()].get();
    //this->WriteNode(*fileStream, dependee);
    //this->WriteConnection(*fileStream, depender, dependee, scopeType);
  }

  if (this->GenerateDependers) {
    auto fileStream = TargetDependersFileStreams[dependee.AsStr()].get();
    //this->WriteNode(*fileStream, depender);
    //this->WriteConnection(*fileStream, depender, dependee, scopeType);
  }
#endif
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
    return !this->GenerateForExternals;
  }

  if (item.Target->GetType() == cmStateEnums::UTILITY) {
    if ((itemName.find("Nightly") == 0) ||
        (itemName.find("Continuous") == 0) ||
        (itemName.find("Experimental") == 0)) {
      return true;
    }
  }

  if (item.Target->IsImported() && !this->GenerateForExternals) {
    return true;
  }

  return !this->TargetTypeEnabled(item.Target->GetType());
}

bool cmDependenciesWriter::ItemNameFilteredOut(std::string const& itemName)
{
  if (itemName == ">") {
    // FIXME: why do we even receive such a target here?
    return true;
  }

  if (cmGlobalGenerator::IsReservedTarget(itemName)) {
    return true;
  }

  for (cmsys::RegularExpression& regEx : this->TargetsToIgnoreRegex) {
    if (regEx.is_valid()) {
      if (regEx.find(itemName)) {
        return true;
      }
    }
  }

  return false;
}

bool cmDependenciesWriter::TargetTypeEnabled(
  cmStateEnums::TargetType targetType) const
{
  switch (targetType) {
    case cmStateEnums::EXECUTABLE:
      return this->GenerateForExecutables;
    case cmStateEnums::STATIC_LIBRARY:
      return this->GenerateForStaticLibs;
    case cmStateEnums::SHARED_LIBRARY:
      return this->GenerateForSharedLibs;
    case cmStateEnums::MODULE_LIBRARY:
      return this->GenerateForModuleLibs;
    case cmStateEnums::INTERFACE_LIBRARY:
      return this->GenerateForInterfaceLibs;
    case cmStateEnums::OBJECT_LIBRARY:
      return this->GenerateForObjectLibs;
    case cmStateEnums::UNKNOWN_LIBRARY:
      return this->GenerateForUnknownLibs;
    case cmStateEnums::UTILITY:
      return this->GenerateForCustomTargets;
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

std::string cmDependenciesWriter::GetEdgeStyle(DependencyType dt)
{
  std::string style;
  switch (dt) {
    case DependencyType::LinkPrivate:
      style = "[ style = " + std::string(GRAPHVIZ_EDGE_STYLE_PRIVATE) + " ]";
      break;
    case DependencyType::LinkInterface:
      style = "[ style = " + std::string(GRAPHVIZ_EDGE_STYLE_INTERFACE) + " ]";
      break;
    default:
      break;
  }
  return style;
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
