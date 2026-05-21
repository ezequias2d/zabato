#pragma once

#include <stdint.h>
#include <zabato/animator_state.hpp>
#include <zabato/object.hpp>
#include <zabato/vector.hpp>

namespace zabato
{

/**
 * @class animator_graph
 * @brief An authored state machine for an animator.
 *
 * Holds states and entry-state index. Persisted as an object_resource
 * (.zfile) via the existing serializer / xml_serializer pipeline. At runtime
 * an animator holds a resource_ref to this graph and drives it.
 * Transitions fire on message triggers and/or exit time — no data-driven
 * parameter conditions. All state logic lives in scripts.
 */
class animator_graph : public object
{
public:
    static const rtti TYPE;
    const rtti &type() const override { return TYPE; }
    static void reflect(reflection &r);

    animator_graph()           = default;
    ~animator_graph() override = default;

    vector<pointer<animator_state>> &states() { return m_states; }
    const vector<pointer<animator_state>> &states() const { return m_states; }

    /** @brief Transitions evaluated every frame regardless of current state. */
    vector<animator_transition> &any_state_transitions()
    {
        return m_any_state_transitions;
    }
    const vector<animator_transition> &any_state_transitions() const
    {
        return m_any_state_transitions;
    }

    size_t entry_state_index() const { return m_entry_state_index; }
    void set_entry_state_index(size_t i) { m_entry_state_index = i; }

    /** @brief Find a state index by name. Returns SIZE_MAX if not found. */
    size_t find_state_index(const symbol_ref &name) const;

    void save(class serializer &s) const override;
    void load(class serializer &s, class serializer_link *link) override;
    void save_xml(class xml_serializer &s,
                  tinyxml2::XMLElement &el) const override;
    void load_xml(class xml_serializer &s, tinyxml2::XMLElement &el) override;

private:
    vector<pointer<animator_state>> m_states;
    vector<animator_transition> m_any_state_transitions;
    size_t m_entry_state_index = 0;
};

} // namespace zabato
