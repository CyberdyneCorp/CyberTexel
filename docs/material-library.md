# Material graph library

`ctex/graph/material_library.hpp` stores named material-graph presets without a
device, executor, or filesystem dependency. A preset carries a caller-assigned
stable identity, display name, optional thumbnail resource identifier, and a
complete `GraphDocument`.

Libraries keep presets ordered by stable identity and reject duplicates. Their
`CTEX_MATERIAL_LIBRARY` representation is versioned and canonical: insertion
order cannot change its bytes, arbitrary UTF-8 and graph bytes are hex encoded,
and every embedded graph is validated by the normal graph deserializer. A host
can therefore transfer the same library to another machine, resolve the same
stable identity, and instantiate an independent graph with identical content.

This graph-only format establishes the `material-graph` preset contract. Asset
packing, shelf metadata, version migration, origin recording, and portable
resource resolution belong to the later `smart-materials` capability.
