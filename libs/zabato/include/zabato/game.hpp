#pragma once

#include "object.hpp"
#include <zabato/vector.hpp>

namespace zabato
{
class game
{
public:
    static size_t s_start_objects;
    typedef bool (*initializer_t)(void);

    static void add_initializer(initializer_t initializer)
    {
        if (!m_initializers)
            m_initializers = new vector<initializer_t>();
        m_initializers->push_back(initializer);
    }

    static void initialize()
    {
        s_start_objects = object::s_in_use.size();
        if (s_start_objects > 0)
        {
            object::print_in_use("app_log.txt",
                                 "Objects in use at game initialization");
            assert(s_start_objects == 0);
        }

        bool failed = false;

        if (m_initializers)
            for (auto initializer : *m_initializers)
            {
                if (!initializer())
                    failed = true;
            }

        delete m_initializers;
        m_initializers  = nullptr;
        s_start_objects = object::s_in_use.size();

        assert(!failed);
    }

    typedef bool (*terminator_t)(void);
    static void add_terminator(terminator_t terminator)
    {
        if (!m_terminators)
            m_terminators = new vector<terminator_t>();
        m_terminators->push_back(terminator);
    }

    static void terminate()
    {
        bool failed = false;
        if (m_terminators)
            for (auto terminator : *m_terminators)
            {
                if (!terminator())
                    failed = true;
            }
        delete m_terminators;
        m_terminators = nullptr;

        assert(!failed);
    }

private:
    static vector<initializer_t> *m_initializers;
    static vector<terminator_t> *m_terminators;
};
}; // namespace zabato