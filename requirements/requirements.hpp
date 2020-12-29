#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <vector>

// Requirements is a class that handles pairs of objects for which the first object depends on the second object.
// Pairs are ensured to be unique.
// By default reflexivity is not allowed (objects that depend on each other) but it can be activated at construction.
// Because it does not have any sense, the status of reflexivity can not be changed after instantiation.

template <typename T>
class Requirements
{
public:
    Requirements() = delete;
    Requirements(const bool reflexive = false) noexcept
        : m_reflexive(reflexive) {};

    bool reflexive() const noexcept { return m_reflexive; }

    void clear() noexcept { m_requirements.clear(); }

    bool empty() const noexcept { return m_requirements.empty(); }
    size_t size() const noexcept { return m_requirements.size(); }

    void add(const T& dependent, const T& requirement);
    void remove(const T& dependent, const T& requirement);
    void remove_dependent(const T& dependent);
    void remove_requirement(const T& requirement);
    void remove_all(const T& object);
    bool exists(const T& dependent, const T& requirement) const noexcept;               // check direct requirement
    bool requires(const T& dependent, const T& requirement) const;                      // check in requirements chains
    bool depends(const T& requirement, const T& dependent) const;                       // check in dependents chains
    bool has_requirements(const T& dependent) const noexcept;
    bool has_dependents(const T& requirement) const noexcept;
    std::vector<T> requirements(const T& dependent) const;                                  // lists direct requirements of dependent
    std::vector<T> dependents(const T& requirement) const;                                  // lists direct dependents of requirement
    std::vector<std::vector<T>> all_requirements(const T& dependent) const;                 // returns all requirements of dependent in chains
    std::vector<std::vector<T>> all_dependencies(const T& requirement) const;               // returns all dependencies of requirement in chains
    std::vector<std::vector<T>> all_requirements(bool without_duplicates = true) const;       // returns all chains of requirements
    std::vector<std::vector<T>> all_dependencies(bool without_duplicates = true) const;       // returns all chains of dependencies
    std::unordered_multimap<T, T> get() const;                                          // returns a copy of the table of requirements
    void set(const std::unordered_multimap<T, T>& requirements);                        // initialize the table of requirements with the one provided, performing checks
    void merge(const std::unordered_multimap<T, T>& requirements);                      // append the table provided to the existing table of requirements

private:
    std::unordered_multimap<T, T> m_requirements{};
    bool m_reflexive{ false };

    bool _requires(const T& dependent, const T& requirement, const T* prev) const;
    bool _depends(const T& resuirement, const T& dependent, const T* prev) const;
};

// Implementation of templates classes and functions

template <typename T>
void Requirements<T>::add(const T& dependent, const T& requirement)
{
    assert(!(dependent == requirement) && "A requirement can't be requested for object itself.");
    // we must ensure the implicit requirement does not already exist
    assert(!requires(dependent, requirement) && "(Implicit) requirement is already defined.");
    if (!m_reflexive)
        // opposite requirement is only allowed if reflexivity is activated, directly or indirectly
        assert(!requires(requirement, dependent) && "Opposite requirement cannot be set while reflexivity is not allowed.");
    m_requirements.insert({ dependent, requirement });
}

template <typename T>
void Requirements<T>::remove(const T& dependent, const T& requirement)
{
    auto range = m_requirements.equal_range(dependent);
    auto itr = range.first;
    bool found{ false };
    while (itr != range.second)
    {
        if ((*itr).second == requirement)
        {
            itr = m_requirements.erase(itr);
            found = true;
        }
        else
            ++itr;
    }
    assert(found && "Requirement does not exist.");
}

template <typename T>
void Requirements<T>::remove_dependent(const T& dependent)
{
    auto range = m_requirements.equal_range(dependent);
    assert(range.first != range.second && "No requirement exists for this argument.");
    m_requirements.erase(range.first, range.second);
}

template <typename T>
void Requirements<T>::remove_requirement(const T& requirement)
{
    auto itr = m_requirements.begin();
    bool found{ false };
    while (itr != m_requirements.end())
    {
        if ((*itr).second == requirement)
        {
            itr = m_requirements.erase(itr);
            found = true;
        }
        else
            ++itr;
    }
    assert(found && "No requirement exists for this argument.");
}

template <typename T>
void Requirements<T>::remove_all(const T& object)
{
    if (has_requirements(object))
        remove_dependent(object);
    if (has_dependents(object))
        remove_requirement(object);
}

template <typename T>
bool Requirements<T>::exists(const T& dependent, const T& requirement) const noexcept
{
    bool found{ false };
    auto range = m_requirements.equal_range(dependent);
    auto itr = range.first;
    while (!found && itr != range.second)
    {
        if ((*itr).second == requirement)
            found = true;
        ++itr;
    }
    return found;
}

template <typename T>
bool Requirements<T>::_requires(const T& dependent, const T& requirement, const T* prev) const
{
    bool result = exists(dependent, requirement);
    if (!result && has_requirements(dependent))
    {
        auto reqs = requirements(dependent);
        auto req = reqs.begin();
        while (!result && req != reqs.end())
        {
            if (prev == nullptr || !(*req == *prev))
                result = _requires(*req, requirement, &dependent);
            ++req;
        }
    }
    return result;
}

template <typename T>
bool Requirements<T>::requires(const T& dependent, const T& requirement) const
{
    return _requires(dependent, requirement, nullptr);
}

template <typename T>
bool Requirements<T>::_depends(const T& requirement, const T& dependent, const T* prev) const
{
    bool result = exists(dependent, requirement);
    if (!result && has_dependents(requirement))
    {
        auto deps = dependents(requirement);
        auto dep = deps.begin();
        while (!result && dep != deps.end())
        {
            if (prev == nullptr || !(*dep == *prev))
                result = _depends(*dep, dependent, &requirement);
            ++dep;
        }
    }
    return result;
}

template <typename T>
bool Requirements<T>::depends(const T& requirement, const T& dependent) const
{
    return _depends(requirement, dependent, nullptr);
}

template <typename T>
bool Requirements<T>::has_requirements(const T& dependent) const noexcept
{
    auto itr = m_requirements.find(dependent);
    return itr != m_requirements.end();
}

template <typename T>
bool Requirements<T>::has_dependents(const T& requirement) const noexcept
{
    bool found{ false };
    auto itr = m_requirements.begin();
    while (!found && itr != m_requirements.end())
    {
        if ((*itr).second == requirement)
            found = true;
        ++itr;
    }
    return found;
}

template <typename T>
std::vector<T> Requirements<T>::requirements(const T& dependent) const
{
    std::vector<T> result{};
    auto range = m_requirements.equal_range(dependent);
    auto itr = range.first;
    while (itr != range.second)
    {
        result.push_back((*itr).second);
        ++itr;
    }
    return result;
}

template <typename T>
std::vector<T> Requirements<T>::dependents(const T& requirement) const
{
    std::vector<T> result{};
    auto itr = m_requirements.begin();
    while (itr != m_requirements.end())
    {
        if ((*itr).second == requirement)
            result.push_back((*itr).first);
        ++itr;
    }
    return result;
}

template <typename T>
std::vector<std::vector<T>> Requirements<T>::all_requirements(const T& dependent) const
{
    assert(has_requirements(dependent) && "No requirement exists for this argument.");
    std::vector<std::vector<T>> result{};
    auto reqs = requirements(dependent);
    for (auto req : reqs)
    {
        std::vector<T> row{ dependent };
        size_t count{ 0 };
        if (has_requirements(req))
        {
            auto subreqs = all_requirements(req);
            for (auto sub : subreqs)
            {
                if (sub[1] != dependent)         // avoid infinite loop while reflexivity is active
                {
                    for (auto itr : sub)
                        row.push_back(itr);
                    result.push_back(row);
                    row.clear();
                    row.push_back(dependent);
                    ++count;
                }
            }
        }
        if (count == 0)
        {
            row.push_back(req);
            result.push_back(row);
        }
    }
    return result;
}

template <typename T>
std::vector<std::vector<T>> Requirements<T>::all_dependencies(const T& requirement) const
{
    assert(has_dependents(requirement) && "No dependent exists for this argument.");
    std::vector<std::vector<T>> result{};
    auto deps = dependents(requirement);
    for (auto dep : deps)
    {
        std::vector<T> row{ requirement };
        size_t count{ 0 };
        if (has_dependents(dep))
        {
            auto subdeps = all_dependencies(dep);
            for (auto sub : subdeps)
            {
                if (sub[1] != requirement)       // avoid infinite loops while reflexivity is active
                {
                    for (auto itr : sub)
                        row.push_back(itr);
                    result.push_back(row);
                    row.clear();
                    row.push_back(requirement);
                    ++count;
                }
            }
        }
        if (count == 0)
        {
            row.push_back(dep);
            result.push_back(row);
        }
    }
    return result;
}

template <typename T>
std::vector<std::vector<T>> Requirements<T>::all_requirements(bool without_duplicates) const
{
    std::vector<std::vector<T>> result{};
    auto itr = m_requirements.begin();
    while (itr != m_requirements.end())
    {
        if (!without_duplicates || without_duplicates && !has_dependents((*itr).first))
        {
            bool exists{ false };
            auto res = result.begin();
            while (!exists && res != result.end())
            {
                if ((*res)[0] == (*itr).first)
                    exists = true;
                ++res;
            }
            if (!exists)
            {
                auto deps = all_requirements((*itr).first);
                for (auto dep : deps)
                    result.push_back(dep);
            }
        }
        ++itr;
    }
    return result;
}

template <typename T>
std::vector<std::vector<T>> Requirements<T>::all_dependencies(bool without_duplicates) const
{
    std::vector<std::vector<T>> result{};
    auto itr = m_requirements.begin();
    while (itr != m_requirements.end())
    {
        if (!without_duplicates || without_duplicates && !has_requirements((*itr).second))
        {
            bool exists{ false };
            auto res = result.begin();
            while (!exists && res != result.end())
            {
                if ((*res)[0] == (*itr).second)
                    exists = true;
                ++res;
            }
            if (!exists)                                // Each requirement must be processed one time to avoid duplicates
            {
                auto reqs = all_dependencies((*itr).second);
                for (auto req : reqs)
                    result.push_back(req);
            }
        }
        ++itr;
    }
    return result;
}

template <typename T>
std::unordered_multimap<T, T> Requirements<T>::get() const
{
    std::unordered_multimap<T, T> result{};
    auto itr = m_requirements.begin();
    while (itr != m_requirements.end())
    {
        result.insert({ (*itr).first, (*itr).second });
        ++itr;
    }
    return result;
}

template <typename T>
void Requirements<T>::set(const std::unordered_multimap<T, T>& requirements)
{
    clear();
    merge(requirements);
}

template <typename T>
void Requirements<T>::merge(const std::unordered_multimap<T, T>& requirements)
{
    auto itr = requirements.begin();
    while (itr != requirements.end())
    {
        add((*itr).first, (*itr).second);
        ++itr;
    }
}
