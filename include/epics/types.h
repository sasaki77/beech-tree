#pragma once
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace bchtree::epics {

using PVScalarValue = std::variant<int32_t,     // DBF_LONG
                                   float,       // DBF_FLOAT
                                   double,      // DBF_DOUBLE
                                   uint16_t,    // DBF_ENUM (index)
                                   std::string  // DBF_STRING
                                   >;

// COMMON array alternatives
using PVArrayValue =
    std::variant<std::vector<int32_t>,     // DBF_LONG
                 std::vector<float>,       // DBF_FLOAT
                 std::vector<double>,      // DBF_DOUBLE
                 std::vector<uint16_t>,    // DBF_ENUM array (rare)
                 std::vector<std::string>  // DBF_STRING
                 >;

struct PVMeta {
    uint32_t severity = 0;
    uint32_t status = 0;
    std::chrono::system_clock::time_point timestamp{};
};

struct PVData {
    std::variant<PVScalarValue, PVArrayValue> value;
    // PVScalarValue value;
    PVMeta meta;
    size_t count = 0;
};

}  // namespace bchtree::epics
