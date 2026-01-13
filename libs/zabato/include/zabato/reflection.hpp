#pragma once

#include <zabato/hash_map.hpp>
#include <zabato/string.hpp>
#include <zabato/value.hpp>

namespace zabato
{

struct property_def
{
    value getter;
    value setter;
};

struct reflection
{
    hash_map<string, value> methods;
    hash_map<string, property_def> properties;
    value constructor;
    value destructor;

    void add_method(const char *name, value method)
    {
        methods.add_or_set(name, method);
    }

    void add_property(const char *name, value getter, value setter = value())
    {
        properties.add_or_set(name, {getter, setter});
    }
};

} // namespace zabato
