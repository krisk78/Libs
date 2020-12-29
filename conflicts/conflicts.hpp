#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include "../requirements/requirements.hpp"

// Class conflicts implements a specialized container that lists a relationship between 2 objects, x <-> y.
// Create a relationship with an object itself is not allowed.
// Doubles are not allowed, each relationship is unique.

template <typename T>
class Conflicts
{
    public:
        Conflicts()
            : m_cascading(false) {};
        Conflicts(const bool cascading)
            : m_cascading(cascading) {};

        bool cascading() { return m_cascading; }
        void clear() noexcept { m_conflicts.clear(); }

        bool empty() const noexcept { return m_conflicts.empty(); }
        size_t size() const noexcept { return m_conflicts.size(); }

        void add(const T &object1, const T &object2);
        void remove(const T &object1, const T &object2);
        void remove(const T &object);
        bool in_conflict(const T &object) const noexcept;
        bool in_conflict(const T &object1, const T &object2) const noexcept;
        std::vector<T> conflicts(const T &object) const;                            // lists direct conflicts
        std::vector<T> all_conflicts(const T &object) const;                        // lists all implicit conflicts if cascading is on
        std::unordered_multimap<T, T> get() const;
        void set(const std::unordered_multimap<T, T> &conflicts);
        void merge(const std::unordered_multimap<T, T> &conflicts);

    private:
        Requirements<T> m_conflicts{ false };
        bool m_cascading{ false };
        // if cascading is on, an object in conflict with another object is in conflict with all objects in conflict with this object

        bool deep_search(const T& object1, const T& object2, const T* prev = NULL) const noexcept;
        std::vector<T> all_conflicts(const T& object, const T* prev) const;
};

// Implementation of templates functions

template <typename T>
void Conflicts<T>::add(const T &object1, const T &object2)
{
    assert(!(object1 == object2) && "An object can't be in conflict with itself.");
    // we must ensure the conflict does not already exists, directly or, if cascading is on, indirectly
    assert(!in_conflict(object1, object2) && "Conflict already exists.");
    m_conflicts.add(object1, object2);
}

template <typename T>
void Conflicts<T>::remove(const T &object1, const T &object2)
{
    // conflict definition is searched for the 2 directions (object, conflict) and (conflict, object)
    bool found {false};
    if (m_conflicts.exists(object1, object2))
    {
        m_conflicts.remove(object1, object2);
        found = true;
    }
    if (m_conflicts.exists(object2, object1))
    {
        m_conflicts.remove(object2, object1);
        found = true;
    }
    assert(found && "Conflict does not exist.");
}

template <typename T>
void Conflicts<T>::remove(const T &object)
{
    assert(in_conflict(object) && "Conflict does not exist.");
    m_conflicts.remove_all(object);
}

template <typename T>
bool Conflicts<T>::in_conflict(const T &object) const noexcept
{
    return  m_conflicts.has_requirements(object) || m_conflicts.has_dependents(object);
}

template <typename T>
bool Conflicts<T>::deep_search(const T &object1, const T &object2, const T* prev) const noexcept
{
    bool result = m_conflicts.exists(object1, object2) || m_conflicts.exists(object2, object1);
    if (!result && m_cascading)
    {
        // Deep search : if 2 objects are in conflict then a third one that is in conflict with one of them is in conflict with the other
        auto requirements = m_conflicts.requirements(object1);
        auto req = requirements.begin();
        while (!result && req != requirements.end())
        {
            if (prev == nullptr || !(*req == *prev || *req == object2))
                result = deep_search(*req, object2, &object1);
            ++req;
        }
        if (!result)
        {
            auto dependents = m_conflicts.dependents(object1);
            auto dep = dependents.begin();
            while (!result && dep != dependents.end())
            {
                if (prev == nullptr || !(*dep == *prev || *dep == object2))
                    result = deep_search(*dep, object2, &object1);
                ++dep;
            }
        }
    }
    return result;
}

template <typename T>
bool Conflicts<T>::in_conflict(const T &object1, const T &object2) const noexcept
{
    bool result = deep_search(object1, object2);
    if (!result)
        result = deep_search(object2, object1);
    return result;
}

template <typename T>
std::vector<T> Conflicts<T>::conflicts(const T &object) const
{
    std::vector<T> result {};
    auto confs = m_conflicts.requirements(object);
    for (auto con : confs)
        result.push_back(con);
    confs = m_conflicts.dependents(object);
    for (auto con : confs)
        result.push_back(con);
    return result;
}

template <typename T>
std::vector<T> Conflicts<T>::all_conflicts(const T &object, const T* prev) const
{
    std::vector<T> result {};
    auto confs = m_conflicts.requirements(object);
    for (auto con : confs)
    {
        if (prev == nullptr || !(con == *prev))
        {
            result.push_back(con);
            if (m_cascading)
            {
                // perform a recursive search if cascading is on
                auto res = all_conflicts(con, &object);
                for (auto itr : res)
                    result.push_back(itr);
            }
        }
    }
    confs = m_conflicts.dependents(object);
    for (auto con : confs)
    {
        if (prev == nullptr || !(con == *prev))
        {
            result.push_back(con);
            if (m_cascading)
            {
                // perform a recursive search if cascading is on
                auto res = all_conflicts(con, &object);
                for (auto itr : res)
                    result.push_back(itr);
            }
        }
    }
    return result;
}

template <typename T>
std::vector<T> Conflicts<T>::all_conflicts(const T &object) const
{
    return all_conflicts(object, nullptr);
}

template <typename T>
std::unordered_multimap<T, T> Conflicts<T>::get() const
{
    return m_conflicts.get();
}

template <typename T>
void Conflicts<T>::set(const std::unordered_multimap<T, T> &conflicts)
{
    clear();
    merge(conflicts);
}

template <typename T>
void Conflicts<T>::merge(const std::unordered_multimap<T, T> &conflicts)
{
    auto itr = conflicts.begin();
    while (itr != conflicts.end())
    {
        add((*itr).first, (*itr).second);
        ++itr;
    }
}
