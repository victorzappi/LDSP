// adapted from here: https://stackoverflow.com/questions/28828957/enum-to-string-in-modern-c11-c14-c17-and-future-c20

#ifndef ENUM_H_
#define ENUM_H_

#include <cstddef>      // For size_t.
#include <cstring>      // For strcspn, strncpy.
#include <stdexcept>    // For runtime_error.



// A "typical" mapping macro. MAP(macro, a, b, c, ...) expands to
// macro(a) macro(b) macro(c) ...
// The helper macro COUNT(a, b, c, ...) expands to the number of
// arguments, and IDENTITY(x) is needed to control the order of
// expansion of __VA_ARGS__ on Visual C++ compilers.
#define MAP(macro, ...) \
    IDENTITY( \
        APPLY(CHOOSE_MAP_START, COUNT(__VA_ARGS__)) \
            (macro, __VA_ARGS__))

#define CHOOSE_MAP_START(count) MAP ## count

#define APPLY(macro, ...) IDENTITY(macro(__VA_ARGS__))

#define IDENTITY(x) x

#define MAP1(m, x)      m(x)
#define MAP2(m, x, ...) m(x) IDENTITY(MAP1(m, __VA_ARGS__))
#define MAP3(m, x, ...) m(x) IDENTITY(MAP2(m, __VA_ARGS__))
#define MAP4(m, x, ...) m(x) IDENTITY(MAP3(m, __VA_ARGS__))
#define MAP5(m, x, ...) m(x) IDENTITY(MAP4(m, __VA_ARGS__))
#define MAP6(m, x, ...) m(x) IDENTITY(MAP5(m, __VA_ARGS__))
#define MAP7(m, x, ...) m(x) IDENTITY(MAP6(m, __VA_ARGS__))
#define MAP8(m, x, ...) m(x) IDENTITY(MAP7(m, __VA_ARGS__))

//VIC extended num of enums from 8 to 16
#define MAP9(m, x, ...) m(x) IDENTITY(MAP8(m, __VA_ARGS__))
#define MAP10(m, x, ...) m(x) IDENTITY(MAP9(m, __VA_ARGS__))
#define MAP11(m, x, ...) m(x) IDENTITY(MAP10(m, __VA_ARGS__))
#define MAP12(m, x, ...) m(x) IDENTITY(MAP11(m, __VA_ARGS__))
#define MAP13(m, x, ...) m(x) IDENTITY(MAP12(m, __VA_ARGS__))
#define MAP14(m, x, ...) m(x) IDENTITY(MAP13(m, __VA_ARGS__))
#define MAP15(m, x, ...) m(x) IDENTITY(MAP14(m, __VA_ARGS__))
#define MAP16(m, x, ...) m(x) IDENTITY(MAP15(m, __VA_ARGS__))

//VIC extended num of enums from 8 to 16
#define EVALUATE_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, count, ...) \
    count

//VIC extended num of enums from 8 to 16
#define COUNT(...) \
    IDENTITY(EVALUATE_COUNT(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)) 



// The type "T" mentioned above that drops assignment operations.
template <typename U>
struct ignore_assign {
    constexpr explicit ignore_assign(U value) : _value(value) { }
    constexpr operator U() const { return _value; }

    constexpr const ignore_assign& operator =(int dummy) const
        { return *this; }

    U   _value;
};



// Prepends "(ignore_assign<_underlying>)" to each argument.
#define IGNORE_ASSIGN_SINGLE(e) (ignore_assign<_underlying>)e,
#define IGNORE_ASSIGN(...) \
    IDENTITY(MAP(IGNORE_ASSIGN_SINGLE, __VA_ARGS__))

// Stringizes each argument.
#define STRINGIZE_SINGLE(e) #e,
#define STRINGIZE(...) IDENTITY(MAP(STRINGIZE_SINGLE, __VA_ARGS__))



// Some helpers needed for _from_string.
constexpr const char    terminators[] = " =\t\r\n";

// The size of terminators includes the implicit '\0'.
constexpr bool is_terminator(char c, size_t index = 0)
{
    return
        index >= sizeof(terminators) ? false :
        c == terminators[index] ? true :
        is_terminator(c, index + 1);
}

constexpr bool matches_untrimmed(const char *untrimmed, const char *s,
                                 size_t index = 0)
{
    return
        is_terminator(untrimmed[index]) ? s[index] == '\0' :
        s[index] != untrimmed[index] ? false :
        matches_untrimmed(untrimmed, s, index + 1);
}



// The macro proper.
//
// There are several "simplifications" in this implementation, for the
// sake of brevity. First, we have only one viable option for declaring
// constexpr arrays: at namespace scope. This probably should be done
// two namespaces deep: one namespace that is likely to be unique for
// our little enum "library", then inside it a namespace whose name is
// based on the name of the enum to avoid collisions with other enums.
// I am using only one level of nesting.
//
// Declaring constexpr arrays inside the struct is not viable because
// they will need out-of-line definitions, which will result in
// duplicate symbols when linking. This can be solved with weak
// symbols, but that is compiler- and system-specific. It is not
// possible to declare constexpr arrays as static variables in
// constexpr functions due to the restrictions on such functions.
//
// Note that this prevents the use of this macro anywhere except at
// namespace scope. Ironically, the C++98 version of this, which can
// declare static arrays inside static member functions, is actually
// more flexible in this regard. It is shown in the CodeProject
// article.
//
// Second, for compilation performance reasons, it is best to separate
// the macro into a "parametric" portion, and the portion that depends
// on knowing __VA_ARGS__, and factor the former out into a template.
//
// Third, this code uses a default parameter in _from_string that may
// be better not exposed in the public interface.

//VIC added function _from_index(...) to return enum using index

#define ENUM(EnumName, Underlying, ...)                               \
namespace data_ ## EnumName {                                         \
    using _underlying = Underlying;                                   \
    enum { __VA_ARGS__ };                                             \
                                                                      \
    constexpr const size_t           _size =                          \
        IDENTITY(COUNT(__VA_ARGS__));                                 \
                                                                      \
    constexpr const _underlying      _values[] =                      \
        { IDENTITY(IGNORE_ASSIGN(__VA_ARGS__)) };                     \
                                                                      \
    constexpr const char * const     _raw_names[] =                   \
        { IDENTITY(STRINGIZE(__VA_ARGS__)) };                         \
}                                                                     \
                                                                      \
struct EnumName {                                                     \
    using _underlying = Underlying;                                   \
    enum _enum : _underlying { __VA_ARGS__ };                         \
                                                                      \
    const char * _to_string() const                                   \
    {                                                                 \
        for (size_t index = 0; index < data_ ## EnumName::_size;      \
             ++index) {                                               \
                                                                      \
            if (data_ ## EnumName::_values[index] == _value)          \
                return _trimmed_names()[index];                       \
        }                                                             \
                                                                      \
        throw std::runtime_error("invalid value");                    \
    }                                                                 \
                                                                      \
    static EnumName _from_index(size_t index)                         \
    {                                                                 \
        return  (EnumName)(_enum)data_ ## EnumName::_values[index];   \
    }                                                                 \
                                                                      \
    constexpr static EnumName _from_string(const char *s,             \
                                           size_t index = 0)          \
    {                                                                 \
        return                                                        \
            index >= data_ ## EnumName::_size ?                       \
                    throw std::runtime_error("invalid identifier") :  \
            matches_untrimmed(                                        \
                data_ ## EnumName::_raw_names[index], s) ?            \
                    (EnumName)(_enum)data_ ## EnumName::_values[      \
                                                            index] :  \
            _from_string(s, index + 1);                               \
    }                                                                 \
                                                                      \
    EnumName() = delete;                                              \
    constexpr EnumName(_enum value) : _value(value) { }               \
    constexpr operator _enum() const { return (_enum)_value; }        \
                                                                      \
  private:                                                            \
    _underlying     _value;                                           \
                                                                      \
    static const char * const * _trimmed_names()                      \
    {                                                                 \
        static char     *the_names[data_ ## EnumName::_size];         \
        static bool     initialized = false;                          \
                                                                      \
        if (!initialized) {                                           \
            for (size_t index = 0; index < data_ ## EnumName::_size;  \
                 ++index) {                                           \
                                                                      \
                size_t  length =                                      \
                    std::strcspn(data_ ## EnumName::_raw_names[index],\
                                 terminators);                        \
                                                                      \
                the_names[index] = new char[length + 1];              \
                                                                      \
                std::strncpy(the_names[index],                        \
                             data_ ## EnumName::_raw_names[index],    \
                             length);                                 \
                the_names[index][length] = '\0';                      \
            }                                                         \
                                                                      \
            initialized = true;                                       \
        }                                                             \
                                                                      \
        return the_names;                                             \
    }                                                                 \
};

#endif /* ENUM_H_ */
