# stuff_sack

Efficient multi-language message packing / serialization. Targeted at lightweight or embedded
applications where runtime performance and memory usage are key.

If you have worked on applications where embedded devices need to send and recieve messages between
each other and possibly a host machine, you have probably hand written and re-written packing and
unpacking functions, possibly in multiple languages.  `stuff_sack` provides a single source of truth
across languages and devices for message definitions while ensuring both sides are operating on a
consistent (up-to-date) definition before unpacking.

Generated libraries currently implemented for:

* **C** - without dynamic memory allocation
* **C++** - with and without dynamic memory allocation
* **Python**

## Table of Contents

* [Examples and Test](#examples-and-test)
* [Message Specification](#message-specification)
  + [Document Start / Format](#document-start--format)
  + [Primitive Types](#primitive-types)
  + [Bitfields](#bitfields)
  + [Enums](#enums)
  + [Structs](#structs)
    - [Type Descriptions](#type-descriptions)
  + [Messages](#messages)
* [Generating / Building Libraries](#generating--building-libraries)
  + [Viewing Generated Library Documentation](#viewing-generated-library-documentation)
* [Implementation Details](#implementation-details)
  + [Messages](#messages-1)
  + [Packing](#packing)

## Examples and Test

The library is built using [Bazel](https://bazel.build/).  You can run the test suites for all of
the generated libraries like so:

```Shell
bazel test //test/...
```

You can find examples for:
* `BUILD` file -- [test/BUILD](test/BUILD).
* Message definitions -- [test/test_message_spec.yaml](test/test_message_spec.yaml)
* C library usage -- [test/test_c_stuff_sack.c](test/test_c_stuff_sack.c)
* C++ library usage -- [test/test_cc_stuff_sack.c](test/test_cc_stuff_sack.cc)
* Python library usage -- [test/test_py_stuff_sack.py](test/test_py_stuff_sack.py)

You can build and view the documentation for the generated libraries like so (or view a snapshot
[here](https://agoessling.github.io/stuff_sack/)):

```Shell
bazel run //test:test_message_def-docs.view
```

## Message Specification

Messages and their contained types are described in [YAML](https://yaml.org/) specifications.  For a
working example see [test/test_message_spec.yaml](test/test_message_spec.yaml).

### Document Start / Format

Every `stuff_sack` message specification starts with a YAML document start `---` and is followed by a
single dictionary of types:

```YAML
---
TypeName1:
  ...

TypeName2:
  ...
```

### Primitive Types

The following primitive types are available to be used within array, struct, and message
definitions:

* `uint8` - unsigned 1 byte integer
* `uint16` - unsigned 2 byte integer
* `uint32` - unsigned 4 byte integer
* `uint64` - unsigned 8 byte integer
* `int8` - signed 1 byte integer
* `int16` - signed 2 byte integer
* `int32` - signed 4 byte integer
* `int64` - signed 8 byte integer
* `bool` - 1 byte boolean
* `float` - 4 byte single precision floating point
* `double` - 8 byte double precision floating point

### Bitfields

Required entries:

* **type** (string) - Always `Bitfield`.
* **fields** (array) - Ordered listing of bitfield fields.
  * **Array Element** (dictionary) - Field data.
    * **field name: number of bits** - (string): (integer)

```YAML
BitfieldTypeName:
  type: Bitfield
  fields:
    - field_name_1: 5
    - field_name_2: 3
```

Bitfields must be less than or equal to 64 bits.  The smallest type that will accomodate the
specified bits will be used (e.g. 1, 2, 4, or 8 bytes).

### Enums

Required entries:

* **type** (string) - Always `Enum`.
* **values** (array) - Ordered listing of enum values.
  * **Array Element** (dictionary) - Value data.
    * **enum value name: null** - (string): (null | empty)

```YAML
EnumTypeName:
  type: Enum
  values:
    - value_name_1:
    - value_name_2: null
```

Enum values start at 0 and increment with each specified value.  Assigning arbitrary values is
currently not supported.  Enums are always stored as signed integers.  The smallest type that will
accomodate the specified values will be used (e.g. 1, 2, 4, or 8 bytes).

### Structs

Required entries:

* **type** (string) - Always `Struct`.
* **fields** (array) - Ordered list of struct fields.
  * **Array Element** (dictionary) - Field data.
    * **field name: type description** - (string): (string | array)

#### Type Descriptions

Type descriptions can be either a string or array.  When the type description is a string it must be
the name of a primitive type or a previously defined custom type.  When the type description is an
array it must be a two element array where the first element is itself a type description and the
second element is an integer length. Multi-dimensional arrays can be described by using an array
type description within another array type description: `[[int8, 2], 3]`.

```YAML
StructTypeName1:
  type: Struct
  fields:
    - field_name_1: int8

StructTypeName2:
  type: Struct
  fields:
    - field_name_1: uint16
    - field_name_2: [StructTypeName1, 10]
    - field_name_3: [[uint8, 3], 8]
```

### Messages

Messages are just a special case of Structs.  They are the smallest unit that can be serialized /
packed and de-serialized / unpacked.  They share the same required entries as Structs except that
their `type` entry is always `Message`.

```YAML
MessageName:
  type: Message
  fields:
    - field_name_1: uint8
    - field_name_2: [uint8, 2]
```

## Generating / Building Libraries

`stuff_sack` utilizes [Bazel](https://bazel.build/) to build the generated libraries and message
definitions for the supported languages.  To incorporate `stuff_sack` into your project add the
following to your `WORKSPACE` file.

```Starlark
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "stuff_sack",
    # See release page for latest version url and sha.
)

load("@stuff_sack//tools:level_1_repositories.bzl", "stuff_sack_level_1_deps")
stuff_sack_level_1_deps()

load("@stuff_sack//tools:level_2_repositories.bzl", "stuff_sack_level_2_deps")
stuff_sack_level_2_deps()

load("@stuff_sack//tools:level_3_repositories.bzl", "stuff_sack_level_3_deps")
stuff_sack_level_3_deps()
```

Then within the relevant `BUILD` file within your project use the `gen_message_def` macro to
generate and build the libraries and utilities associated with a message specification YAML file.

```Starlark
load("@stuff_sack//tools:gen_message_def.bzl", "gen_message_def")

gen_message_def(
    name = "test_message_def",
    message_spec = "test_message_spec.yaml",
    ...
)
```

The macro has arguments:

* `name` - target name to be used as basename of various outputs.
* `message_spec` - YAML message definition.
* `...` - all further arguments (e.g. `visibility`) are passed as `**kwargs` to the resulting
  `cc_library` and `py_library` outputs.

The macro generates several outputs:

* `cc_library` - resulting C library with type definitions and pack / unpack functions.
  * Output target name suffix: "-c" e.g. `test_message_def-c`.
* `cc_library` - resulting C++ library with type definitions and pack / unpack functions.
  * Output target name suffix: "-cc" e.g. `test_message_def-cc`.
* `py_library` - resulting Python library with type definitions and pack / unpack functions.
  * Output target name suffix: "-py" e.g. `test_message_def-py`.
* `sphinx_html` - generated [Sphinx](https://github.com/agoessling/rules_sphinx) documentation for
  resulting libraries.
  * Output target name suffix: "-docs" e.g. `test_message_def-docs`.

### Viewing Generated Library Documentation

The generated documentation is the best place to review the generated library type definitions and
API.  The `sphinx_html` output contains a `.view` action verb  which can be invoked to view
generated documentation.

```Shell
bazel run //path/to:target_name-docs.view
```

## Implementation Details

### Messages

Messages are the smallest entity that can be packed and unpacked through the generated API.  For the
most part they are very similar to Structs except that a header (`ss_header`) is implicitly
prepended to the field list:

```YAML
SsHeader: (implicitly defined)
  type: Struct
  fields:
    - uid: uint32
    - len: uint16

MessageName:
  type: Message
  fields:
    - ss_header: SsHeader (implicitly added)
    - field_name_1: field_type_1
    - field_name_2: field_type_2
    ...
```

The header does not need to be manipulated by the user.  It is automatically populated by the
packing function.  The `uid` field is a unique identifier that denotes not only what message the
byte sequence represents, but also the exact naming and structure of it's definition.  This ensures
that the unpacking function (e.g. on a host machine) only succeeds if it is operating on the exact
same version of the message as the function that packed it (e.g. on an embedded device).  The `uid`
is generated by hashing the relevant parts of the message definition (and any contained type
definitions).  The `len` field is the packed size of the message.

### Packing

Messages are packed in Big Endian order without any padding.  For example:

```YAML
MessageName:
  type: Message
  fields:
    - first: int8
    - second: uint32
```

would be packed into the byte sequence:

```ASCII
 ┌─uid──┬─uid──┬─uid──┬─uid──┬─len──┬─len──┐first─┬─sec.─┬─sec.─┬─sec.─┬─sec.─┐
 │byte 3│byte 2│byte 1│byte 0│byte 1│byte 0│byte 0│byte 3│byte 2│byte 1│byte 0│
 └──0───┴──1───┴──2───┴──3───┴──4───┴──5───┴──6───┴──7───┴──8───┴──9───┴──10──┘
```
