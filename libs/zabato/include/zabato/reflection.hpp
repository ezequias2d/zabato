#pragma once

#include <zabato/hash_map.hpp>
#include <zabato/string.hpp>
#include <zabato/value.hpp>
#include <zabato/vector.hpp>

namespace zabato
{

struct property_def
{
    value getter;
    value setter;
    value attributes;
};

struct reflection
{
    hash_map<string, value> methods;
    hash_map<string, property_def> properties;
    vector<string> property_order;
    value constructor;
    value destructor;

    void add_method(const char *name, value method)
    {
        methods.add_or_set(name, method);
    }

    void add_property(const char *name,
                      value getter,
                      value setter     = value(),
                      value attributes = value())
    {
        properties.add_or_set(name, {getter, setter, attributes});
        property_order.push_back(name);
    }
};

} // namespace zabato
