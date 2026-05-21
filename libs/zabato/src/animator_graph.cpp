#include <tinyxml2.h>

#include <zabato/animator_graph.hpp>
#include <zabato/reflection.hpp>
#include <zabato/script.hpp>
#include <zabato/serializer.hpp>
#include <zabato/xml_serializer.hpp>

namespace zabato
{

const rtti animator_graph::TYPE("zabato.animator_graph",
                                &object::TYPE,
                                animator_graph::reflect);

void animator_graph::reflect(reflection &r) { object::reflect(r); }

size_t animator_graph::find_state_index(const symbol_ref &name) const
{
    for (size_t i = 0; i < m_states.size(); ++i)
        if (m_states[i] && m_states[i]->state_name() == name)
            return i;
    return (size_t)-1;
}

void animator_graph::save(serializer &s) const
{
    object::save(s);
    s.write((uint64_t)m_entry_state_index);
    s.write((uint64_t)m_states.size());
    for (const auto &st : m_states)
        s.write((const object *)st.get());
    s.write((uint64_t)m_any_state_transitions.size());
    for (const auto &t : m_any_state_transitions)
    {
        s.write((uint64_t)t.dst_state_index);
        s.write(string_view(t.message_trigger.c_str()));
        s.write((double)t.exit_time_norm);
        s.write((double)t.duration);
        s.write((uint8_t)(t.has_exit_time ? 1 : 0));
        s.write((uint8_t)(t.interruptible ? 1 : 0));
    }
}

void animator_graph::load(serializer &s, serializer_link *link)
{
    object::load(s, link);

    uint64_t idx = 0;
    s.read(idx);
    m_entry_state_index = (size_t)idx;

    uint64_t scount = 0;
    s.read(scount);
    m_states.resize(scount);
    for (uint64_t i = 0; i < scount; ++i)
    {
        object *ref = nullptr;
        s.read(ref);
        link->add_child_id(ref);
    }

    uint64_t acount = 0;
    s.read(acount);
    m_any_state_transitions.resize(acount);
    for (auto &t : m_any_state_transitions)
    {
        uint64_t tidx = 0;
        s.read(tidx);
        t.dst_state_index = (size_t)tidx;

        string trig;
        s.read(trig);
        t.message_trigger = symbol_ref(trig.c_str());

        double etn = 1.0;
        s.read(etn);
        t.exit_time_norm = (real)etn;

        double dur = 0.0;
        s.read(dur);
        t.duration = (real)dur;

        uint8_t het = 0;
        s.read(het);
        t.has_exit_time = het != 0;

        uint8_t ipt = 1;
        s.read(ipt);
        t.interruptible = ipt != 0;
    }
}

void animator_graph::save_xml(xml_serializer &s, tinyxml2::XMLElement &el) const
{
    object::save_xml(s, el);
    el.SetAttribute("entry_state", (int64_t)m_entry_state_index);

    tinyxml2::XMLElement *states = el.GetDocument()->NewElement("states");
    for (const auto &st : m_states)
    {
        tinyxml2::XMLElement *se = el.GetDocument()->NewElement("state");
        s.write_object(*se, st.get());
        states->InsertEndChild(se);
    }
    el.InsertEndChild(states);

    tinyxml2::XMLElement *ax = el.GetDocument()->NewElement("any_state");
    for (const auto &t : m_any_state_transitions)
    {
        tinyxml2::XMLElement *te = el.GetDocument()->NewElement("transition");
        te->SetAttribute("dst_state", (int64_t)t.dst_state_index);
        if (!t.message_trigger.empty())
            te->SetAttribute("message", t.message_trigger.c_str());
        te->SetAttribute("exit_time_norm", (double)t.exit_time_norm);
        te->SetAttribute("duration", (double)t.duration);
        te->SetAttribute("has_exit_time", t.has_exit_time);
        te->SetAttribute("interruptible", t.interruptible);
        ax->InsertEndChild(te);
    }
    el.InsertEndChild(ax);
}

void animator_graph::load_xml(xml_serializer &s, tinyxml2::XMLElement &el)
{
    object::load_xml(s, el);
    int64_t idx = 0;
    el.QueryInt64Attribute("entry_state", &idx);
    m_entry_state_index = (size_t)idx;

    m_states.clear();
    if (auto *states = el.FirstChildElement("states"))
    {
        for (tinyxml2::XMLElement *se = states->FirstChildElement("state"); se;
             se                       = se->NextSiblingElement("state"))
        {
            pointer<object> o = s.read_object(*se);
            auto *st          = c_dynamic_cast<animator_state>(o.get());
            if (st)
                m_states.push_back(pointer<animator_state>(st));
        }
    }

    m_any_state_transitions.clear();
    if (auto *ax = el.FirstChildElement("any_state"))
    {
        for (tinyxml2::XMLElement *te = ax->FirstChildElement("transition"); te;
             te                       = te->NextSiblingElement("transition"))
        {
            animator_transition t;
            int64_t tidx = 0;
            te->QueryInt64Attribute("dst_state", &tidx);
            t.dst_state_index = (size_t)tidx;

            const char *msg   = te->Attribute("message");
            t.message_trigger = symbol_ref(msg ? msg : "");

            double etn = 1.0;
            te->QueryDoubleAttribute("exit_time_norm", &etn);
            t.exit_time_norm = (real)etn;

            double dur = 0.0;
            te->QueryDoubleAttribute("duration", &dur);
            t.duration = (real)dur;

            bool het = false;
            te->QueryBoolAttribute("has_exit_time", &het);
            t.has_exit_time = het;

            bool ipt = true;
            te->QueryBoolAttribute("interruptible", &ipt);
            t.interruptible = ipt;

            m_any_state_transitions.push_back(t);
        }
    }
}

} // namespace zabato
