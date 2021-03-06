/*! \file usage.cpp
    \brief Defines the functions for the static library.
    \author Christophe COUAILLET
*/

#include "pch.h"

#include "../utils/utils.hpp"
#include "usage.hpp"

std::string AType_toStr(Argument_Type arg)
{
    static const std::array<const std::string, 3> AType_Label{ "string", "boolean", "simple" };
    return AType_Label[(int)arg];
}

Argument::Argument(const std::string& name)
{
    m_name = name;
}

Argument::Argument(const Argument& argument)
{
    m_name = argument.m_name;
    m_required = argument.m_required;
    helpstring = argument.helpstring;
    for (auto val : argument.value)
        value.push_back(val);
}

Argument::Argument(const Argument* argument)
{
    m_name = argument->m_name;
    m_required = argument->m_required;
    helpstring = argument->helpstring;
    for (auto val : argument->value)
        value.push_back(val);
}

std::string Argument::name() const noexcept
{
    return m_name;
}

std::ostream& operator<<(std::ostream& os, const Argument& arg)
{   // used to return the xml definition of the Argument
    return arg.print(os);
}

std::ostream& Argument::print(std::ostream& os, const std::string& indent) const
{
    os << indent << "<name>" << m_name << "</name>" << std::endl;
    os << indent << "<helpstring>" << helpstring << "</helpstring>" << std::endl;
    os << indent << "<required>" << std::boolalpha << m_required << "</required>" << std::endl;
    return os;
}

Named_Arg::Named_Arg(const Named_Arg& argument) : Argument(argument)
{
    switch_char = argument.switch_char;
    m_type = argument.m_type;
    m_default_value = argument.m_default_value;
}

Named_Arg::Named_Arg(const Named_Arg* argument) : Argument(argument)
{
    switch_char = argument->switch_char;
    m_type = argument->m_type;
    m_default_value = argument->m_default_value;
}

std::ostream& Named_Arg::print(std::ostream& os, const std::string& indent) const
{
    os << indent << "<named>" << std::endl;
    Argument::print(os, indent + "\t");
    os << indent << "\t<switch_char>" << switch_char << "</switch_char>" << std::endl;
    os << indent << "\t<type>" << (int)m_type << "</type>" << std::endl;
    os << indent << "\t<default_value>" << m_default_value << "</default_value>" << std::endl;
    os << indent << "</named>" << std::endl;
    return os;
}

std::ostream& Unnamed_Arg::print(std::ostream& os, const std::string& indent) const
{
    os << indent << "<unnamed>" << std::endl;
    Argument::print(os, indent + "\t");
    os << indent << "\t<many>" << std::boolalpha << many << "</many>" << std::endl;
    os << indent << "</unnamed>" << std::endl;
    return os;
}

void Named_Arg::set_required(const bool required)
{
    assert((!required || required && m_default_value.empty()) && "An argument can't be required if it defines a default value.");
    m_required = required;
}

void Named_Arg::set_type(Argument_Type type)
{
    assert((type != Argument_Type::simple || type == Argument_Type::simple && m_default_value.empty()) && "Type simple can't be set for arguments with a default value.");
    m_type = type;
}

void Named_Arg::set_default_value(const std::string& default_value)
{
    assert((default_value.empty() || !default_value.empty() && !m_required) && "A default value can't be set for a required argument.");
    assert((default_value.empty() || !default_value.empty() && m_type != Argument_Type::simple) && "A default value can't be set for an argument of type simple.");
    m_default_value = default_value;
}

Usage::Usage(const std::string& prog_name)
{
    program_name = prog_name;
}

Usage::~Usage()
{
    for (auto arg : m_arguments)
        delete arg.second;      // args are dynamically created when added
}

void Usage::add_Argument(const Argument& argument)
{
    auto itr = m_arguments.find(argument.name());
    assert(itr == m_arguments.end() && "Argument already exists.");
    Argument* arg;
    if (argument.named())
        arg = new Named_Arg{ dynamic_cast<const Named_Arg*>(&argument) };
    else
        arg = new Unnamed_Arg{ dynamic_cast<const Unnamed_Arg*>(&argument) };
    m_arguments[argument.name()] = arg;
    m_argsorder.push_back(m_arguments[argument.name()]);
    m_syntax_valid = false;
}

void Usage::remove_Argument(const std::string& name)
{
    auto itr = m_arguments.find(name);
    assert(itr != m_arguments.end() && "Unknown argument name.");
    auto ord = std::find(m_argsorder.begin(), m_argsorder.end(), (*itr).second);
    if (ord != m_argsorder.end())
    {
        m_requirements.remove_all(*ord);
        m_conflicts.remove(*ord);
        m_argsorder.erase(ord);
    }
    Argument* arg = (*itr).second;
    m_arguments.erase(name);
    delete arg;         // was dynamically created when added
    m_syntax_valid = false;
}

void Usage::remove_all() noexcept
{
    // we must free memory for each dynamically created arg
    while (!m_arguments.empty())
        remove_Argument(m_arguments[0]->name());
    m_syntax_valid = false;
}

void Usage::clear() noexcept
{
    remove_all();
    program_name.clear();
    description.clear();
    usage.clear();
    m_syntax_valid = false;
}

Argument* Usage::get_Argument(const std::string& name)
{
    auto arg_itr = m_arguments.find(name);
    if (arg_itr != m_arguments.end())
        return (*arg_itr).second;
    return NULL;
}

std::vector<Argument*> Usage::get_Arguments()
{
    std::vector<Argument*> result{};
    for (auto arg : m_argsorder)
        result.push_back(arg);
    return result;
}

std::unordered_map<std::string, std::vector<std::string>> Usage::get_values() const
{
    std::unordered_map<std::string, std::vector<std::string>> result{};
    for (auto arg : m_argsorder)
        result[arg->name()] = arg->value;
    return result;
}

std::vector<std::string> Usage::get_values(const std::string& name) const
{
    auto itr = m_arguments.find(name);
    assert(itr != m_arguments.end() && "Unknown argument name.");
    return (*itr).second->value;
}

void Usage::add_requirement(const std::string& dependent, const std::string& requirement)
{
    assert((!dependent.empty() && !requirement.empty()) && "Requirements cannot be created without arguments name.");
    assert((dependent != requirement) && "An argument cannot require itself.");
    auto itr1 = m_arguments.find(dependent);
    auto itr2 = m_arguments.find(requirement);
    assert((itr1 != m_arguments.end() && itr2 != m_arguments.end()) && "Unknown argument name.");
    // define a requirement on a unrequired argument for a required argument has no sense - yes, it can be used to differentiate [a b] and [a [b]]
    // assert((!(*itr1).second->required() || (*itr1).second->required() == (*itr2).second->required()) && "A required argument can't depend on an unrequired argument.");
    // define a requirement has no sense for arguments in conflict
    assert(!(m_conflicts.in_conflict((*itr1).second, (*itr2).second)) && "A requirement can not be set for arguments in conflict.");
    // we must ensure the pair does not already exist
    assert(!m_requirements.exists((*itr1).second, (*itr2).second) && "Requirement is already defined.");
    m_requirements.add((*itr1).second, (*itr2).second);
    m_syntax_valid = false;
}

void Usage::remove_requirement(const std::string& dependent, const std::string& requirement)
{
    auto itr1 = m_arguments.find(dependent);
    auto itr2 = m_arguments.find(requirement);
    assert((itr1 != m_arguments.end() && itr2 != m_arguments.end()) && "Unknown argument name.");
    assert(m_requirements.exists((*itr1).second, (*itr2).second) && "Requirement does not exist.");
    m_requirements.remove((*itr1).second, (*itr2).second);
    m_syntax_valid = false;
}

void Usage::remove_requirements(const std::string& argument)
{
    auto itr = m_arguments.find(argument);
    assert(itr != m_arguments.end() && "Unknown argument name.");
    assert(m_requirements.has_requirements((*itr).second) && "No requirement exists for this argument.");
    m_requirements.remove_requirement((*itr).second);
    m_syntax_valid = false;
}

void Usage::clear_requirements() noexcept
{
    m_requirements.clear();
    m_syntax_valid = false;
}

bool Usage::requirement_exists(const std::string& dependent, const std::string& requirement) const
{
    auto const itr1 = m_arguments.find(dependent);
    auto const itr2 = m_arguments.find(requirement);
    assert((itr1 != m_arguments.end() && itr2 != m_arguments.end()) && "Unknown argument name.");
    return m_requirements.exists((*itr1).second, (*itr2).second);
}

bool Usage::has_requirements(const std::string& dependent) const
{
    auto itr = m_arguments.find(dependent);
    assert((itr != m_arguments.end()) && "Unknown argument name.");
    return m_requirements.has_requirements((*itr).second);
}

bool Usage::has_dependents(const std::string& requirement) const
{
    auto itr = m_arguments.find(requirement);
    assert((itr != m_arguments.end()) && "Unknown argument name.");
    return m_requirements.has_dependents((*itr).second);
}

Argument* Usage::get_requirement(const std::string& dependent, const std::string& requirement)
{
    auto itr1 = m_arguments.find(dependent);
    auto itr2 = m_arguments.find(requirement);
    assert((itr1 != m_arguments.end() && itr2 != m_arguments.end()) && "Unknown argument name.");
    assert(m_requirements.exists((*itr1).second, (*itr2).second) && "Requirement does not exist.");
    return (*itr2).second;
}

std::vector<std::string> Usage::get_requirements(const std::string& argument) const
{
    auto itr = m_arguments.find(argument);
    assert(itr != m_arguments.end() && "Unknown argument name.");
    std::vector<std::string> result{};
    std::vector<const Argument*> requirements{ m_requirements.requirements((*itr).second) };
    for (auto req : requirements)
        result.push_back(req->name());
    return result;
}

std::vector<std::string> Usage::get_dependents(const std::string& argument) const
{
    auto itr = m_arguments.find(argument);
    assert(itr != m_arguments.end() && "Unknown argument name.");
    std::vector<std::string> result{};
    std::vector<const Argument*> dependents{ m_requirements.dependents((*itr).second) };
    for (auto dep : dependents)
        result.push_back(dep->name());
    return result;
}

std::unordered_multimap<std::string, std::string> Usage::get_requirements() const
{
    std::unordered_multimap<std::string, std::string> result{};
    auto requirements = m_requirements.get();
    auto itr = requirements.begin();
    while (itr != requirements.end())
    {
        result.insert({ (*itr).first->name(), (*itr).second->name() });
        ++itr;
    }
    return result;
}

void Usage::set_requirements(const std::unordered_multimap<std::string, std::string>& requirements)
{
    auto itr = requirements.begin();
    while (itr != requirements.end())
    {
        add_requirement((*itr).first, (*itr).second);
        ++itr;
    }
    m_syntax_valid = false;
}

void Usage::add_conflict(const std::string& arg1, const std::string& arg2)
{
    assert((!arg1.empty() && !arg2.empty()) && "Conflicts cannot be created without arguments name.");
    assert((arg1 != arg2) && "An argument cannot be in conflict with itself.");
    auto itr1 = m_arguments.find(arg1);
    auto itr2 = m_arguments.find(arg2);
    assert((itr1 != m_arguments.end() && itr2 != m_arguments.end()) && "Unknown argument name.");
    assert(((*itr1).second->required() == (*itr2).second->required()) && "All arguments in conflict must be either required or unrequired.");
    // set a conflict for arguments linked by a requirement has no sense
    assert(!(m_requirements.requires((*itr1).second, (*itr2).second) || m_requirements.requires((*itr2).second, (*itr1).second)) && "Dependent arguments cannot be in conflict.");
    // we must ensure the pair does not already exist for the 2 directions (name, conflict) and (conflict, name)
    assert(!(m_conflicts.in_conflict((*itr1).second, (*itr2).second)) && "Conflict already exists.");
    m_conflicts.add((*itr1).second, (*itr2).second);
    m_syntax_valid = false;
}

void Usage::remove_conflict(const std::string& arg1, const std::string& arg2)
{
    auto itr1 = m_arguments.find(arg1);
    auto itr2 = m_arguments.find(arg2);
    assert((itr1 != m_arguments.end() && itr2 != m_arguments.end()) && "Unknown argument name.");
    // conflict definition is searched for the 2 directions (name, conflict) and (conflict, name)
    assert(m_conflicts.in_conflict((*itr1).second, (*itr2).second) && "Conflict does not exist.");
    m_conflicts.remove((*itr1).second, (*itr2).second);
    m_syntax_valid = false;
}

void Usage::remove_conflicts(const std::string& argument)
{
    auto itr = m_arguments.find(argument);
    assert(itr != m_arguments.end() && "Unknown argument name.");
    // search must be done for 2 directions (name, x) and (x, name)
    assert(m_conflicts.in_conflict((*itr).second) && "No conflict exists for this argument.");
    m_conflicts.remove((*itr).second);
    m_syntax_valid = false;
}

void Usage::clear_conflicts() noexcept
{
    m_conflicts.clear();
    m_syntax_valid = false;
}

bool Usage::in_conflict(const std::string& argument) const
{
    auto itr = m_arguments.find(argument);
    assert(itr != m_arguments.end() && "Unknown argument name.");
    return m_conflicts.in_conflict((*itr).second);
}

bool Usage::in_conflict(const std::string& arg1, const std::string& arg2) const
{
    auto itr1 = m_arguments.find(arg1);
    auto itr2 = m_arguments.find(arg2);
    assert((itr1 != m_arguments.end() && itr2 != m_arguments.end()) && "Unknown argument name.");
    return m_conflicts.in_conflict((*itr1).second, (*itr2).second);
}

Argument* Usage::get_conflict(const std::string& arg1, const std::string& arg2)
{
    auto itr1 = m_arguments.find(arg1);
    auto itr2 = m_arguments.find(arg2);
    assert((itr1 != m_arguments.end() && itr2 != m_arguments.end()) && "Unknown argument name.");
    assert(m_conflicts.in_conflict((*itr1).second, (*itr2).second) && "These arguments are not in conflict.");
    return (*itr2).second;
}

std::vector<std::string> Usage::get_conflicts(const std::string& argument) const
{
    auto itr = m_arguments.find(argument);
    assert(itr != m_arguments.end() && "Unknown argument name.");
    std::vector<std::string> result{};
    auto conflicts = m_conflicts.conflicts((*itr).second);
    for (auto con : conflicts)
        result.push_back(con->name());
    return result;
}

std::unordered_multimap<std::string, std::string> Usage::get_conflicts() const
{
    auto conflicts = m_conflicts.get();
    std::unordered_multimap<std::string, std::string> result{};
    auto itr = conflicts.begin();
    while (itr != conflicts.end())
    {
        result.insert({ (*itr).first->name(), (*itr).second->name() });
        ++itr;
    }
    return result;
}

void Usage::set_conflicts(const std::unordered_multimap<std::string, std::string>& conflicts)
{
    auto itr = conflicts.begin();
    while (itr != conflicts.end())
    {
        add_conflict((*itr).first, (*itr).second);
        ++itr;
    }
    m_syntax_valid = false;
}

void Usage::load_from_file(const std::string& fname)
{
    m_syntax_valid = true;
}

void Usage::save_to_file(const std::string& fname) const
{}

void Usage::set_syntax(const std::string& syntax)
{
    m_syntax_string = syntax;
    m_syntax_valid = true;
}

std::string Usage::set_parameters(int argc, char* argv[])
{
    static const char* SYNTAX_ERROR{ "Error found in command line argument number %i: '%s' - see %s /? for help." };
    static const char* TYPE_MISMATCH{ "Argument '%s' passed as '%s' while expected type is '%s' - see %s /? for help." };
    static const char* UNKNOW_ARGUMENT{ "Unknown argument '/%s' - see %s /? for help." };
    static const char* REQUIRED_ARGUMENT{ "Missing required argument '%s' - see %s /? for help." };
    static const char* CONFLICT{ "Arguments '%s' and '%s' can't be used together - see %s /? for help." };
    if (argc == 0)
        return "No argument to evaluate.";
    std::vector<bool> set_args(m_argsorder.size(), false);
    bool many{ false };
    size_t unnamed{ 0 };
    for (size_t i = 1; i < (size_t)argc; i++)
    {
        std::string p = argv[i];
        if (p.empty())
            continue;
        bool named{ p[0] == '/' };
        if (named)
            p.erase(0, 1);
        if (p.empty())
            return get_message(SYNTAX_ERROR, i, argv[i], program_name.c_str());
        if (p == "?")
            // Help requested
            return "?";
        auto quote = p.find('\"');
        bool quoted{ quote != std::string::npos };
        std::string value{};
        if (!named)
        {
            value = p;
            if (quoted)
            {
                // TODO format value inside quotes
            }
            if (many)
                m_argsorder[unnamed]->value.push_back(value);
            else
            {
                bool found{ false };
                for (size_t i = 0; i < m_argsorder.size(); i++)
                {
                    if (!m_argsorder[i]->named() && !set_args[i])
                    {
                        m_argsorder[i]->value.push_back(value);
                        set_args[i] = true;
                        many = dynamic_cast<Unnamed_Arg*>(m_argsorder[i])->many;
                        unnamed = i;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    return get_message(SYNTAX_ERROR, i, argv[i], program_name.c_str());
            }
            continue;
        }
        many = false;
        if (quoted)
        {
            value = p.substr(quote + 1, p.length() - quote - 1);
            p.erase(quote, p.length() - 1);
            // TODO format value inside quotes
        }
        if (p.empty())
            return get_message(SYNTAX_ERROR, i, argv[i], program_name.c_str());
        Argument_Type type_p{ Argument_Type::simple };
        auto colon = p.find(':');
        if (colon != std::string::npos)
        {
            if (colon < p.size() - 1)
                value = p.substr(colon + 1, p.size() - colon) + value;
            type_p = Argument_Type::string;
            p.erase(colon, p.length() - colon);
        }
        else
        {
            auto sgn = p[p.length() - 1];
            if (sgn == '+' || sgn == '-')
            {
                type_p = Argument_Type::boolean;
                p.erase(p.length() - 1);
                if (!value.empty())
                    return get_message(SYNTAX_ERROR, i, argv[i], program_name.c_str());
                value = "false";
                if (sgn == '+')
                    value = "true";
            }
            else
            {
                // simple argument
                if (!value.empty())
                    return get_message(SYNTAX_ERROR, i, argv[i], program_name.c_str());
                value = "true";
            }
        }
        if (p.empty())
            return get_message(SYNTAX_ERROR, i, argv[i], program_name.c_str());
        bool found{ false };
        for (size_t i = 0; i < m_argsorder.size(); i++)
        {
            std::string name2{};
            if (m_argsorder[i]->named() && !set_args[i])
            {
                name2 = dynamic_cast<Named_Arg*>(m_argsorder[i])->switch_char;
                if (p == m_argsorder[i]->name() || p == name2)
                {
                    Argument_Type type_a = dynamic_cast<Named_Arg*>(m_argsorder[i])->type();
                    if (type_p != type_a)
                        return get_message(TYPE_MISMATCH, m_argsorder[i]->name().c_str(),
                            AType_toStr(type_p).c_str(), AType_toStr(type_a).c_str(), program_name.c_str());
                    m_argsorder[i]->value.push_back(value);
                    set_args[i] = true;
                    found = true;
                    break;
                }
            }
        }
        if (!found)
            return get_message(UNKNOW_ARGUMENT, p.c_str(), program_name.c_str());
    }
    for (size_t i = 0; i < m_argsorder.size(); i++)
    {
        if (!set_args[i] && m_argsorder[i]->required())
        {
            // inspect args in direct conflict with the current one
            auto cons = m_conflicts.conflicts(m_argsorder[i]);
            bool con_defined{ false };
            for (auto con : cons)
            {
                auto itr = std::find(m_argsorder.begin(), m_argsorder.end(), con);
                if (itr != m_argsorder.end())
                {
                    auto j = std::distance(m_argsorder.begin(), itr);
                    if (set_args[j])
                    {
                        con_defined = true;
                        break;
                    }
                }
            }
            if (!con_defined)
                return get_message(REQUIRED_ARGUMENT, m_argsorder[i]->name().c_str(), program_name.c_str());
        }
        if (!set_args[i] && m_argsorder[i]->named())
        {
            auto dval = dynamic_cast<Named_Arg*>(m_argsorder[i])->default_value();
            if (!dval.empty())
            {
                // default value must be applied only if required args are effectively used
                auto reqs = m_requirements.requirements(m_argsorder[i]);
                bool req_defined{ false };
                if (reqs.empty())
                    req_defined = true;     // always apply default value for non-dependent args
                for (auto req : reqs)
                {
                    auto itr = std::find(m_argsorder.begin(), m_argsorder.end(), req);
                    if (itr != m_argsorder.end())
                    {
                        auto j = std::distance(m_argsorder.begin(), itr);
                        if (set_args[j])
                        {
                            req_defined = true;
                            break;
                        }
                    }
                }
                if (req_defined)
                {
                    m_argsorder[i]->value.push_back(dval);
                    set_args[i] = true;
                }
            }
        }
    }
    for (size_t i = 0; i < m_argsorder.size(); i++)
    {
        if (set_args[i])
        {
            for (size_t j = 0; j < m_argsorder.size(); j++)
            {
                if (i != j)
                {
                    if (set_args[j] && m_conflicts.in_conflict(m_argsorder[i], m_argsorder[j]))
                        return get_message(CONFLICT, m_argsorder[i]->name().c_str(), m_argsorder[j]->name().c_str(), program_name.c_str());
                    if (!set_args[j] && m_requirements.requires(m_argsorder[i], m_argsorder[j]))
                        return get_message(REQUIRED_ARGUMENT, m_argsorder[j]->name().c_str(), program_name.c_str());
                }
            }
        }
    }
    // All is fine and values are affected to arguments
    return "";
}

/* void Usage::create_syntax()
{
    m_syntax_string = program_name + " ";
    auto reqs = m_requirements.get();
    std::vector<std::pair<size_t, size_t>> requirements{};                     // first is the requirement and second the dependent
    std::vector<std::pair<size_t, size_t>> counts(m_argsorder.size(), { 0, 0 });  // first is the count as requirement and second the count as dependent
    for (auto req : reqs)
    {
        auto dep_itr = std::find(m_argsorder.begin(), m_argsorder.end(), req.first);
        auto req_itr = std::find(m_argsorder.begin(), m_argsorder.end(), req.second);
        assert((req_itr != m_argsorder.end() && dep_itr != m_argsorder.end()) && "Unexpected issue in function create_syntax.");
        auto dep_idx = std::distance(m_argsorder.begin(), dep_itr);
        auto req_idx = std::distance(m_argsorder.begin(), req_itr);
        requirements.push_back({ req_idx, dep_idx });
        counts[req_idx].first++;
        counts[dep_idx].second++;
    }
    for (size_t i = 1; i < m_argsorder.size(); i++)
        // adds a requirement with preceding arg for orphan unnamed args
        if (!m_argsorder[i]->named() && !m_requirements.has_requirements(m_argsorder[i]))
        {
            requirements.push_back({ i - 1, i });
            counts[i - 1].first++;
            counts[i].second++;
        }
    std::vector<std::vector<size_t>> conflicts{};
    std::vector<size_t> con_idx(m_argsorder.size(), -1);
    for (size_t i = 0; i < m_argsorder.size(); i++)
    {
        if (con_idx[i] == -1)       // each arg can be in only one group of conflict
        {
            auto cons = m_conflicts.all_conflicts(m_argsorder[i]);
            std::vector<size_t> row{};
            for (auto con : cons)
            {
                auto arg_itr = std::find(m_argsorder.begin(), m_argsorder.end(), con);
                auto arg_idx = std::distance(m_argsorder.begin(), arg_itr);
                row.push_back(arg_idx);
                con_idx[arg_idx] = conflicts.size();
            }
            conflicts.push_back(row);
        }
    }
    auto group_base = counts.size();                       // groups id starts at this index in table counts
    std::vector<std::vector<size_t>> groups{};
    bool transforms{ true };
    while (transforms)
    {
        transforms = false;
        auto req_itr = requirements.begin();
        while (req_itr != requirements.end())
        {
            // links orphan dependents to their requirement
            if (counts[(*req_itr).second].second == 1 && counts[(*req_itr).second].first == 0
                && (counts[(*req_itr).first].first != 0 || counts[(*req_itr).first].second != 0))
            {
                std::vector<size_t> row{ (*req_itr).first, (*req_itr).second };
                auto group_id = group_base + groups.size();
                groups.push_back(row);
                counts.push_back({ 0, 0 });
                auto con1 = con_idx[row[0]];
                auto con2 = con_idx[row[1]];
                assert((con1 == con2 || con1 == -1 || con2 == -1) && "Sorry I don't know how to handle requirements involved in distinct conflicts.");
                if (con1 != -1)
                    con_idx.push_back(con1);
                else
                    con_idx.push_back(con2);
                for (auto req : requirements)
                {
                    if (req.first == row[0])
                    {
                        counts[row[0]].first--;
                        req.first = group_id;
                        counts[group_id].first++;
                    }
                    else if (req.second == row[0])
                    {
                        counts[row[0]].second--;
                        req.second = group_id;
                        counts[group_id].second++;
                    }
                }
                req_itr = requirements.erase(req_itr);
                transforms = true;
            }
            else
                ++req_itr;
        }
        req_itr = requirements.begin();
        while (req_itr != requirements.end())
        {
            // links orphan requirements to their dependent
            if (counts[(*req_itr).first].first == 1 && counts[(*req_itr).first].second == 0
                && counts[(*req_itr).second].first != 0 && counts[(*req_itr).second].second == 1)
            {
                std::vector<size_t> row{ (*req_itr).first, (*req_itr).second };
                auto group_id = group_base + groups.size();
                groups.push_back(row);
                counts.push_back({ 0, 0 });
                counts[row[0]].first--;
                counts[row[1]].second--;
                auto con1 = con_idx[row[0]];
                auto con2 = con_idx[row[1]];
                assert((con1 == con2 || con1 == -1 || con2 == -1) && "Sorry I don't know how to handle requirements involved in distinct conflicts.");
                if (con1 != -1)
                    con_idx.push_back(con1);
                else
                    con_idx.push_back(con2);
                for (auto req : requirements)
                    if (req.first == row[1])
                    {
                        counts[row[1]].first--;
                        req.first = group_id;
                        counts[group_id].first++;
                    }
                req_itr = requirements.erase(req_itr);
                transforms = true;
            }
        }
    }
    size_t start{ 0 };
    if (m_requirements.has_requirements(m_argsorder[0]))
    {
        // search for starting argument
        auto req_init = m_requirements.all_requirements(m_argsorder[0]);
        size_t max_size{ 0 };
        auto req_itr = req_init.begin();
        auto req_found = req_init.end();
        while (req_itr != req_init.end())
        {
            // search for the longest chain
            if ((*req_itr).size() > max_size)
            {
                max_size = (*req_itr).size();
                req_found = req_itr;
            }
            ++req_itr;
        }
        auto arg_itr = std::find(m_argsorder.begin(), m_argsorder.end(), (*req_found).back());
        start = std::distance(m_argsorder.begin(), arg_itr);
    }
    // TODO
    m_syntax_valid = true;
} */

std::ostream& operator<<(std::ostream& os, Usage& us)
{
    if (!us.syntax_is_valid())
    {
        // TODO review with create_syntax
        // us.create_syntax();
    }
    os << us.description << std::endl << std::endl;
    os << "Syntax:" << std::endl;
    os << "    " << us.m_syntax_string << std::endl << std::endl;
    // argsorder is used to list the elements in the same order they were added
    // do a first pass to determine the max length
    size_t max_length{ 0 };
    auto itr = us.m_argsorder.begin();
    while (itr != us.m_argsorder.end())
    {
        auto lgth = (*itr)->name().length();
        if ((*itr)->named() && dynamic_cast<Named_Arg*>(*itr)->switch_char != ' ')
            lgth += 3;
        if (lgth > max_length)
            max_length = lgth;
        ++itr;
    }
    std::string filler{ "" };
    filler.append(max_length, ' ');
    itr = us.m_argsorder.begin();
    while (itr != us.m_argsorder.end())
    {
        os << "    " << (*itr)->name();
        auto lgth = (*itr)->name().length();
        if ((*itr)->named() && dynamic_cast<Named_Arg*>(*itr)->switch_char != ' ')
        {
            os << ", " << dynamic_cast<Named_Arg*>(*itr)->switch_char;
            lgth += 3;
        }
        if (filler.length() > lgth)
        {
            std::string fill2{ "" };
            fill2.append(max_length - lgth, ' ');
            os << fill2;
        }
        // display the helpstring with indent
        std::stringstream ostr;
        ostr.str((*itr)->helpstring);
        std::array<char, 255> buf;
        bool indent{ false };
        while (ostr.getline(&buf[0], 255, '\n'))
        {
            if (indent)
                os << "    " << filler;
            else
                indent = true;
            os << "    " << &buf[0] << std::endl;
        }
        if ((*itr)->named())
        {
            auto dval = dynamic_cast<Named_Arg*>(*itr)->default_value();
            if (!dval.empty())
            {
                os << "    " << filler << "    ";
                if (dval == "\t")
                    os << "'Tab'";
                else if (dval == " ")
                    os << "'Space'";
                else
                    os << "'" << dval << "'";
                os << " by default." << std::endl;
            }
        }
        ++itr;
    }
    os << std::endl;
    os << us.usage << std::endl;
    return os;
}
