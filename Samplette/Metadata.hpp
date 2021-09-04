#pragma once
#include <Process/ProcessMetadata.hpp>

namespace Samplette
{
class Model;
}

PROCESS_METADATA(
    ,
    Samplette::Model,
    "2d9ac779-4f76-4c62-b636-0df0aceb0847",
    "Samplette",                                  // Internal name
    "Samplette",                                  // Pretty name
    Process::ProcessCategory::Other,              // Category
    "Other",                                      // Category
    "Description",                                // Description
    "Author",                                     // Author
    (QStringList{"Put", "Your", "Tags", "Here"}), // Tags
    {},                                           // Inputs
    {},                                           // Outputs
    Process::ProcessFlags::SupportsAll            // Flags
)
