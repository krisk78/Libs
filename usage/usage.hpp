#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../requirements/requirements.hpp"
#include "../conflicts/conflicts.hpp"

enum Argument_Type {
    string,         // passed as Argument:value
    boolean,        // passed as Argument+ or Argument-
    simple          // passed as Argument without additional value
};

class Argument
{
public:
    virtual bool named() const noexcept = 0;
    std::string name() const noexcept;
    std::string helpstring{};
    bool required() const noexcept { return m_required; }
    std::vector<std::string> value{};

    Argument() = delete;
    Argument(const std::string& name);
    Argument(const Argument& argument);
    Argument(const Argument* argument);

    virtual void set_required(const bool required) = 0;

    friend std::ostream& operator<<(std::ostream& os, const Argument& argument);
    // used to return the xml definition of the Argument

protected:
    std::string m_name{};
    bool m_required{ false };

    virtual std::ostream& print(std::ostream& os, const std::string& indent = "") const;
};

class Named_Arg : public Argument
{
    // An argument that is named. Named args can be used in any order.
    // They are always preceded by /
public:

    virtual bool named() const noexcept override { return true; }
    char switch_char{ ' ' };
    Argument_Type type() noexcept { return m_type; }
    std::string default_value() noexcept { return m_default_value; }

    Named_Arg() = delete;
    Named_Arg(const std::string& name) : Argument(name) {};
    Named_Arg(const Named_Arg& argument);
    Named_Arg(const Named_Arg* argument);

    virtual void set_required(const bool required) override;
    void set_type(Argument_Type type);
    void set_default_value(const std::string& default_value);

protected:
    Argument_Type m_type{ simple };
    std::string m_default_value{};

    virtual std::ostream& print(std::ostream& os, const std::string& indent = "") const override;
};

class Unnamed_Arg : public Argument
{
    // An argument that is not named. Unnamed args must be passed in the expected order
public:
    virtual bool named() const noexcept override { return false; }
    bool many{ false };

    Unnamed_Arg() = delete;
    Unnamed_Arg(const std::string& name) : Argument(name) {};
    Unnamed_Arg(const Unnamed_Arg& argument) : Argument(argument) { many = argument.many; }
    Unnamed_Arg(const Unnamed_Arg* argument) : Argument(argument) { many = argument->many; }

    virtual void set_required(const bool required) override { Argument::m_required = required; }

protected:
    virtual std::ostream& print(std::ostream& os, const std::string& indent = "") const override;
};

class Usage
{
private:
    // table of arguments
    std::unordered_map<std::string, Argument*> m_arguments{};

    // this non sorted container is used to keep the order in which arguments are defined, mainly for unnamed arguments
    std::vector<Argument*> m_argsorder{};

    // arguments that requires use of other arguments
    Requirements<const Argument*> m_requirements{ false };

    // arguments delimited by | ; conflicting arguments must be either all required either all optional
    // if they are all required they are delimited by () ; if they are all optional they are delimited by []
    Conflicts<const Argument*> m_conflicts{ true };         // with cascading

    // command line syntax
    std::string m_syntax_string{};

    // build the command line syntax ; build_syntax is called each time the operator << is used
    void create_syntax();               // TODO
    bool m_syntax_valid{ false };

public:
    std::string program_name{};
    std::string description{};
    std::string usage{};

    Usage() = delete;
    Usage(const std::string& prog_name);    // program name is in argv[0] passed to main()
    ~Usage();
    void add_Argument(const Argument& argument);
    void remove_Argument(const std::string& name);
    void remove_all() noexcept;
    void clear() noexcept;
    Argument* get_Argument(const std::string& name);
    std::vector<Argument*> get_Arguments();
    std::unordered_map<std::string, std::vector<std::string>> get_values() const;       // return couples name, values - to use after call of set_parameters
    std::vector<std::string> get_values(const std::string& name) const;                 // return values for a single argument

    void add_requirement(const std::string& dependent, const std::string& requirement);
    void remove_requirement(const std::string& dependent, const std::string& requirement);
    void remove_requirements(const std::string& dependent);
    void clear_requirements() noexcept;
    bool requirement_exists(const std::string& dependent, const std::string& requirement) const;
    bool has_requirements(const std::string& dependent) const;
    bool has_dependents(const std::string& requirement) const;
    Argument* get_requirement(const std::string& dependent, const std::string& requirement);
    std::vector<std::string> get_requirements(const std::string& dependent) const;
    std::vector<std::string> get_dependents(const std::string& requirement) const;
    std::unordered_multimap<std::string, std::string> get_requirements() const;
    void set_requirements(const std::unordered_multimap<std::string, std::string>& requirements);

    void add_conflict(const std::string& arg1, const std::string& arg2);
    void remove_conflict(const std::string& arg1, const std::string& arg2);
    void remove_conflicts(const std::string& argument);
    void clear_conflicts() noexcept;
    bool in_conflict(const std::string& argument) const;
    bool in_conflict(const std::string& arg1, const std::string& arg2) const;
    Argument* get_conflict(const std::string& arg1, const std::string& arg2);
    std::vector<std::string> get_conflicts(const std::string& argument) const;
    std::unordered_multimap<std::string, std::string> get_conflicts() const;
    void set_conflicts(const std::unordered_multimap<std::string, std::string>& conflicts);

    void load_from_file(const std::string& fname);      // TODO
    void save_to_file(const std::string& fname) const;  // TODO
    void set_syntax(const std::string& syntax);
    bool syntax_is_valid() const noexcept { return m_syntax_valid; };
    std::string set_parameters(int argc, char* argv[]);
    // returns the collection of <Argument, value> pairs corresponding to argv, with control of rules
    // returns "" if succeed, or the error message if it fails

    friend std::ostream& operator<<(std::ostream& os, Usage& us);
    // use to print usage help
};
