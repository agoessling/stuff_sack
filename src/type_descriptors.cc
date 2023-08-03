#include "src/type_descriptors.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace ss {

using Type = TypeDescriptor::Type;
using PrimType = TypeDescriptor::PrimType;
using TypeMap = DescriptorBuilder::TypeMap;

std::shared_ptr<TypeDescriptor> DescriptorBuilder::ParseStruct(std::string_view name,
                                                               const YAML::Node& node,
                                                               TypeMap& type_map, bool is_msg) {
  std::shared_ptr<StructDescriptor> structure(new StructDescriptor(name));

  // Add implicit SsHeader struct to messages.
  if (is_msg) {
    structure->AddField("ss_header", type_map.at("SsHeader"));
  }

  // Iterate over "fields" array.
  for (const auto& field_node : node["fields"]) {
    // Iterate over field map.  Only one key should be without a leading _.  All else is metadata.
    for (const auto& field_def_pair : field_node) {
      const std::string field_name = field_def_pair.first.as<std::string>();

      // Skip keys with leading _ as they are metadata.
      if (field_name.rfind("_", 0) == 0) continue;

      const YAML::Node& field_type_node = field_def_pair.second;

      if (field_type_node.IsScalar()) {
        // Field is simple type.
        structure->AddField(field_name, type_map.at(field_type_node.as<std::string>()));
      } else if (field_type_node.IsSequence()) {
        // Field is array.
        structure->AddField(field_name, ParseArray(field_type_node, type_map));
      } else {
        throw std::runtime_error("Unrecognized field description.");
      }
    }
  }

  return structure;
}

std::shared_ptr<TypeDescriptor> DescriptorBuilder::ParseArray(const YAML::Node& node,
                                                              TypeMap& type_map) {
  const YAML::Node& type_node = node[0];  // Type is first element of sequence.
  const int size = node[1].as<int>();  // Size is second.

  std::shared_ptr<TypeDescriptor> array;

  if (type_node.IsScalar()) {
    // Type is simple type.
    array = std::shared_ptr<ArrayDescriptor>(
        new ArrayDescriptor(type_map.at(type_node.as<std::string>()), size));
  } else if (type_node.IsSequence()) {
    // Type is nested array.
    array = std::shared_ptr<ArrayDescriptor>(
        new ArrayDescriptor(ParseArray(type_node, type_map), size));
    ;
  } else {
    throw std::runtime_error("Unrecognized array description.");
  }

  // Array is not already in type_map.
  if (!type_map.count(array->name())) {
    type_map.emplace(array->name(), array);
    return array;
  }

  return type_map.at(array->name());
}

std::shared_ptr<TypeDescriptor> DescriptorBuilder::ParseEnum(std::string_view name,
                                                             const YAML::Node& node) {
  std::shared_ptr<EnumDescriptor> enumerator(new EnumDescriptor(name));

  // Iterate over "values" array.
  for (const auto& value_node : node["values"]) {
    // Iterate over value map.  Only one key should be without a leading _.  All else is metadata.
    for (const auto& value_def_pair : value_node) {
      const std::string value_name = value_def_pair.first.as<std::string>();

      // Ignore metadata keys.
      if (value_name.rfind('_', 0) == 0) continue;

      enumerator->AddValue(value_name);
    }
  }

  return enumerator;
}

std::shared_ptr<TypeDescriptor> DescriptorBuilder::ParseBitfield(std::string_view name,
                                                                 const YAML::Node& node,
                                                                 const TypeMap& type_map) {
  std::shared_ptr<BitfieldDescriptor> structure(new BitfieldDescriptor(name));

  // Iterate over "fields" array.
  for (const auto& field_node : node["fields"]) {
    // Iterate over field map.  Only one key should be without a leading _.  All else is metadata.
    for (const auto& field_def_pair : field_node) {
      const std::string field_name = field_def_pair.first.as<std::string>();

      // Skip keys with leading _ as they are metadata.
      if (field_name.rfind("_", 0) == 0) continue;

      const int size = field_def_pair.second.as<int>();

      std::shared_ptr<TypeDescriptor> prim;
      if (size <= 8) {
        prim = type_map.at("uint8");
      } else if (size <= 16) {
        prim = type_map.at("uint16");
      } else if (size <= 32) {
        prim = type_map.at("uint32");
      } else if (size <= 64) {
        prim = type_map.at("uint64");
      } else {
        throw std::runtime_error("Bitfield field too large.");
      }

      structure->AddField(field_name, prim, size);
    }
  }

  return structure;
}

TypeMap DescriptorBuilder::FromNode(const YAML::Node& root_node) {
  TypeMap type_map;

  // Add base types.
  type_map.emplace("uint8", std::shared_ptr<PrimitiveDescriptor>(
                                new PrimitiveDescriptor("uint8", PrimType::kUint8)));
  type_map.emplace("uint16", std::shared_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("uint16", PrimType::kUint16)));
  type_map.emplace("uint32", std::shared_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("uint32", PrimType::kUint32)));
  type_map.emplace("uint64", std::shared_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("uint64", PrimType::kUint64)));
  type_map.emplace("int8", std::shared_ptr<PrimitiveDescriptor>(
                               new PrimitiveDescriptor("int8", PrimType::kInt8)));
  type_map.emplace("int16", std::shared_ptr<PrimitiveDescriptor>(
                                new PrimitiveDescriptor("int16", PrimType::kInt16)));
  type_map.emplace("int32", std::shared_ptr<PrimitiveDescriptor>(
                                new PrimitiveDescriptor("int32", PrimType::kInt32)));
  type_map.emplace("int64", std::shared_ptr<PrimitiveDescriptor>(
                                new PrimitiveDescriptor("int64", PrimType::kInt64)));
  type_map.emplace("bool", std::shared_ptr<PrimitiveDescriptor>(
                               new PrimitiveDescriptor("bool", PrimType::kBool)));
  type_map.emplace("float", std::shared_ptr<PrimitiveDescriptor>(
                                new PrimitiveDescriptor("float", PrimType::kFloat)));
  type_map.emplace("double", std::shared_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("double", PrimType::kDouble)));

  // Add implicit SsHeader.
  std::shared_ptr<StructDescriptor> ss_header(new StructDescriptor("SsHeader"));
  ss_header->AddField("uid", type_map.at("uint32"));
  ss_header->AddField("len", type_map.at("uint16"));
  type_map.emplace("SsHeader", ss_header);

  // Iterate through top level type definitions.
  for (const auto& pair : root_node) {
    const std::string name = pair.first.as<std::string>();
    const YAML::Node& type_node = pair.second;

    const YAML::Node& type_name_node = type_node["type"];
    // Skip nodes that don't contain a "type" key.
    if (!type_name_node) continue;

    const std::string type_name = type_name_node.as<std::string>();
    if (type_name == "Struct") {
      type_map.emplace(name, ParseStruct(name, type_node, type_map, false));
    } else if (type_name == "Message") {
      type_map.emplace(name, ParseStruct(name, type_node, type_map, true));
    } else if (type_name == "Enum") {
      type_map.emplace(name, ParseEnum(name, type_node));
    } else if (type_name == "Bitfield") {
      type_map.emplace(name, ParseBitfield(name, type_node, type_map));
    } else {
      throw std::runtime_error("Unknown type name.");
    }
  }

  return type_map;
}

std::unordered_map<std::string, std::shared_ptr<TypeDescriptor>> DescriptorBuilder::FromFile(
    const std::string& filename) {
  return FromNode(YAML::LoadFile(filename));
}

std::unordered_map<std::string, std::shared_ptr<TypeDescriptor>> DescriptorBuilder::FromString(
    const std::string& str) {
  return FromNode(YAML::Load(str));
}

};  // namespace ss
