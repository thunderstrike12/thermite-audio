#pragma once
#include <ImReflect.hpp>
#include "engine/core/io.hpp"

template <>
struct ImReflect::type_response<tmt::IO::FileLocation> : ImReflect::Detail::required_response<tmt::IO::FileLocation> {
   private:
    bool _is_dropped = false;

   public:
    /* Setters */
    void dropped() { _is_dropped = true; }
    /* Getters */
    bool is_file_dropped() const { return _is_dropped; }
};

void tag_invoke(ImReflect::ImInput_t, const char* name, tmt::IO::FileLocation& value, ImSettings& settings, ImResponse& response);