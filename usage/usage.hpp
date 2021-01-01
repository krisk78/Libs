#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

/*! \file usage.hpp
*	\brief Implements the classes Argument_Type, Argument, Named_Arg, Unnamed_Arg and Usage.
*   \author Christophe COUAILLET
*/

#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../requirements/requirements.hpp"
#include "../conflicts/conflicts.hpp"

/*! \brief Defines the types of Named_Arg objects:
*   \li string: passed as Argument:value,
*   \li boolean: passed as Argument+ or Argument-,
*   \li simple: passed as Argument without additional value.
*/
enum class Argument_Type {
    string = 0,         // passed as Argument:value
    boolean = 1,        // passed as Argument+ or Argument-
    simple = 2          // passed as Argument without additional value
};

/*! \brief A pure virtual class that defines the base for Named_Arg and Unnamed_Arg. */
class Argument
{
public:
    /*! \brief Returns true for Named_Arg objects, false for Unnamed_Arg objects. */
    virtual bool named() const noexcept = 0;
    /*! \brief Returns the name of the argument that is set at instantiation. */
    std::string name() const noexcept;
    /*! \brief Sets or gets the help string displayed in the usage output. */
    std::string helpstring{};
    /*! \brief Returns true if the argument is obligatory.
    *   \sa Argument::set_required()
    */
    bool required() const noexcept { return m_required; }
    /*! \brief list of values passed for the argument. Unnamed_Arg objects can contain many values. */
    std::vector<std::string> value{};

    Argument() = delete;
    /*! \brief Default constructor that sets the name of the argument. */
    Argument(const std::string& name);
    /*! \brief Copy constructor. */
    Argument(const Argument& argument);
    /*! \brief Move constructor. */
    Argument(const Argument* argument);

    /*! \brief Sets if the argument is obligatory or not. */
    virtual void set_required(const bool required) = 0;

    /*! \brief This override returns in the output stream os the xml definition of the argument. */
    friend std::ostream& operator<<(std::ostream& os, const Argument& argument);
    // used to return the xml definition of the Argument

protected:
    /*! \brief Sets or gets the name of the argument. */
    std::string m_name{};
    /*! \brief Sets or gets the obligatory status of the argument, false by default. */
    bool m_required{ false };

    /*! \brief Prints in the output stream the help string of the argument.
        \param indent Optional string inserted before the argument help.
    */
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
    Argument_Type m_type{ Argument_Type::simple };
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
