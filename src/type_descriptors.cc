#include "src/type_descriptors.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace ss {

using Type = TypeDescriptor::Type;
using PrimType = TypeDescriptor::PrimType;

const TypeDescriptor& DescriptorBuilder::ParseStruct(std::string_view name, const YAML::Node& node, bool is_msg) {
  std::unique_ptr<StructDescriptor> structure(new StructDescriptor(name, is_msg));

  // Add implicit SsHeader struct to messages.
  if (is_msg) {
    structure->AddField("ss_header", *type_map_.at("SsHeader").get());
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
        structure->AddField(field_name, *type_map_.at(field_type_node.as<std::string>()).get());
      } else if (field_type_node.IsSequence()) {
        // Field is array.
        structure->AddField(field_name, ParseArray(field_type_node));
      } else {
        throw std::runtime_error("Unrecognized field description.");
      }
    }
  }

  const TypeDescriptor& ret = *structure.get();
  type_map_.emplace(name, std::move(structure));
  return ret;
}

const TypeDescriptor& DescriptorBuilder::ParseArray(const YAML::Node& node) {
  const YAML::Node& type_node = node[0];  // Type is first element of sequence.
  const int size = node[1].as<int>();  // Size is second.

  std::unique_ptr<TypeDescriptor> array;

  if (type_node.IsScalar()) {
    // Type is simple type.
    array = std::unique_ptr<ArrayDescriptor>(
        new ArrayDescriptor(*type_map_.at(type_node.as<std::string>()).get(), size));
  } else if (type_node.IsSequence()) {
    // Type is nested array.
    array = std::unique_ptr<ArrayDescriptor>(new ArrayDescriptor(ParseArray(type_node), size));
  } else {
    throw std::runtime_error("Unrecognized array description.");
  }

  // Array is not already in type_map.
  if (!type_map_.count(array->name())) {
    TypeDescriptor *ret = array.get();
    type_map_.emplace(array->name(), std::move(array));
    return *ret;
  }

  return *type_map_.at(array->name()).get();
}

const TypeDescriptor& DescriptorBuilder::ParseEnum(std::string_view name, const YAML::Node& node) {
  std::unique_ptr<EnumDescriptor> enumerator(new EnumDescriptor(name));

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

  const TypeDescriptor& ret = *enumerator.get();
  type_map_.emplace(name, std::move(enumerator));
  return ret;
}

const TypeDescriptor& DescriptorBuilder::ParseBitfield(std::string_view name, const YAML::Node& node) {
  std::unique_ptr<BitfieldDescriptor> structure(new BitfieldDescriptor(name));

  // Iterate over "fields" array.
  for (const auto& field_node : node["fields"]) {
    // Iterate over field map.  Only one key should be without a leading _.  All else is metadata.
    for (const auto& field_def_pair : field_node) {
      const std::string field_name = field_def_pair.first.as<std::string>();

      // Skip keys with leading _ as they are metadata.
      if (field_name.rfind("_", 0) == 0) continue;

      const int size = field_def_pair.second.as<int>();

      const TypeDescriptor *prim;
      if (size <= 8) {
        prim = type_map_.at("uint8").get();
      } else if (size <= 16) {
        prim = type_map_.at("uint16").get();
      } else if (size <= 32) {
        prim = type_map_.at("uint32").get();
      } else if (size <= 64) {
        prim = type_map_.at("uint64").get();
      } else {
        throw std::runtime_error("Bitfield field too large.");
      }

      structure->AddField(field_name, *prim, size);
    }
  }

  const TypeDescriptor& ret = *structure.get();
  type_map_.emplace(name, std::move(structure));
  return ret;
}

DescriptorBuilder::DescriptorBuilder(const YAML::Node& root_node) {
  // Add base types.
  type_map_.emplace("uint8", std::unique_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("uint8", PrimType::kUint8)));
  type_map_.emplace("uint16", std::unique_ptr<PrimitiveDescriptor>(
                                  new PrimitiveDescriptor("uint16", PrimType::kUint16)));
  type_map_.emplace("uint32", std::unique_ptr<PrimitiveDescriptor>(
                                  new PrimitiveDescriptor("uint32", PrimType::kUint32)));
  type_map_.emplace("uint64", std::unique_ptr<PrimitiveDescriptor>(
                                  new PrimitiveDescriptor("uint64", PrimType::kUint64)));
  type_map_.emplace("int8", std::unique_ptr<PrimitiveDescriptor>(
                                new PrimitiveDescriptor("int8", PrimType::kInt8)));
  type_map_.emplace("int16", std::unique_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("int16", PrimType::kInt16)));
  type_map_.emplace("int32", std::unique_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("int32", PrimType::kInt32)));
  type_map_.emplace("int64", std::unique_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("int64", PrimType::kInt64)));
  type_map_.emplace("bool", std::unique_ptr<PrimitiveDescriptor>(
                                new PrimitiveDescriptor("bool", PrimType::kBool)));
  type_map_.emplace("float", std::unique_ptr<PrimitiveDescriptor>(
                                 new PrimitiveDescriptor("float", PrimType::kFloat)));
  type_map_.emplace("double", std::unique_ptr<PrimitiveDescriptor>(
                                  new PrimitiveDescriptor("double", PrimType::kDouble)));

  // Add implicit SsHeader.
  std::unique_ptr<StructDescriptor> ss_header(new StructDescriptor("SsHeader", false));
  ss_header->AddField("uid", *type_map_.at("uint32").get());
  ss_header->AddField("len", *type_map_.at("uint16").get());
  type_map_.emplace("SsHeader", std::move(ss_header));

  // Iterate through top level type definitions.
  for (const auto& pair : root_node) {
    const std::string name = pair.first.as<std::string>();
    const YAML::Node& type_node = pair.second;

    const YAML::Node& type_name_node = type_node["type"];
    // Skip nodes that don't contain a "type" key.
    if (!type_name_node) continue;

    const std::string type_name = type_name_node.as<std::string>();
    if (type_name == "Struct") {
      (void)ParseStruct(name, type_node, false);
    } else if (type_name == "Message") {
      const TypeDescriptor& msg = ParseStruct(name, type_node, true);
      uid_lookup_.emplace(msg.uid(), &msg);
    } else if (type_name == "Enum") {
      (void)ParseEnum(name, type_node);
    } else if (type_name == "Bitfield") {
      (void)ParseBitfield(name, type_node);
    } else {
      throw std::runtime_error("Unknown type name.");
    }
  }
}

DescriptorBuilder DescriptorBuilder::FromFile(const std::string& filename) {
  return DescriptorBuilder(YAML::LoadFile(filename));
}

DescriptorBuilder DescriptorBuilder::FromString(const std::string& str) {
  return DescriptorBuilder(YAML::Load(str));
}

};  // namespace ss
