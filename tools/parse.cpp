//-----------------------------------------------------------------------------
// Copyright 2023 Eight Brains Studios, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <stdio.h>

#include "../uitk/io/Directory.cpp"
#include "../uitk/io/File.cpp"
#include "../uitk/io/FileSystemNode.cpp"
#include "../uitk/io/IOError.cpp"

//-----------------------------------------------------------------------------
static const std::set<std::string> kProtectionNames = { "public", "protected", "private" };
static const std::set<std::string> kNativeTypes = { "void", "bool", "char", "short", "int", "long",
                                                    "long long", "unsigned char", "unsigned short",
                                                    "unsigned int", "unsigned long", "unsigned long long",
                                                    "long long unsigned int",
                                                    "float", "double" };

typedef std::string Typename;

enum TypeType { kUnknown, kNative, kEnum, kVariable, kFunction, kClass };
enum class Protection {  // can be used like flags
    kPublic =    (1 << 0),
    kProtected = (1 << 1),
    kPrivate =   (1 << 2)
};

struct Type
{
    std::string name;
    std::string fullName;
    TypeType type;
    std::string modifier; // could be multiple types: Type**&
    bool isConst = false; // conflates 'const T*' and 'T* const' but we do not use the latter

    Type() : type(kUnknown) {}
    Type(TypeType t, const std::string& fn)
        : type(t)
    {
        setFullName(fn);
    }

    void setFullName(const std::string& fn)
    {
        fullName = fn;
        auto idx = fullName.rfind(':');
        if (idx < std::string::npos) {
            name = fullName.substr(idx + 1);
        } else {
            name = fullName;
        }
    }

    virtual std::string identifier() const { return name; }
};

struct Enum : public Type
{
    struct Value
    {
        std::string name;
        std::string value;
        std::string docs;

        Value(const std::string& n, const std::string& v, const std::string& d = "")
            : name(n), value(v), docs(d)
        {}
    };

    std::string name;
    std::vector<Value> values;
    std::set<std::string> qualifiers;
    std::string valueType;  // e.g. enum class Name : valueType { ... }
    std::string docs;

    explicit Enum(const std::string& fullName) : Type(kEnum, fullName) {}

    bool hasQualifier(const std::string& q) const
    {
        return (qualifiers.find(q) != qualifiers.end());
    }

    std::string identifier() const override { return "enum_" + name; }
};

struct Function : public Type
{
    struct Arg {
        Type type;
        std::string name;
        std::string defaultVal;
    };

    Type returnType;
    std::vector<Arg> args;
    Protection protection = Protection::kPublic;
    std::set<std::string> qualifiers;  // virtual, etc. ("= 0" is replaced as "pure"), but NOT const
    std::string docs;
    
    explicit Function(const std::string& fullName) : Type(kFunction, fullName) {}

    bool hasQualifier(const std::string& q) const
    {
        return (qualifiers.find(q) != qualifiers.end());
    }

    std::string identifier() const override
    {
        std::string argstr;
        for (size_t i = 0;  i < args.size();  ++i) {
            argstr += "_" + std::to_string(i) + args[i].type.name;
        }
        return "func_" + name + "_r" + returnType.identifier() + argstr;
    }
};

struct Variable : public Type
{
    Type type;
    std::string defaultVal;
    std::set<std::string> qualifiers;  // e.g. static
    std::string docs;

    Variable(const std::string& fullName, const Type& t)
        : Type(kVariable, fullName)
        , type(t)
    {}

    bool hasQualifier(const std::string& q) const
    {
        return (qualifiers.find(q) != qualifiers.end());
    }

    std::string identifier() const override { return "var_" + name; }

};

struct Class : public Type
{
    template <typename T>
    struct Definition {
        std::shared_ptr<T> type;
        Protection protection;

        Definition(std::shared_ptr<T> t, Protection p) : type(t), protection(p) {}
    };

    struct Typedef {
        std::string name;
        std::string targetType;

        Typedef(const std::string& n, const std::string& tt) : name(n), targetType(tt) {}
    };

    std::string super;
    std::map<std::string, std::shared_ptr<Definition<Enum>>> enums;
    std::map<std::string, std::shared_ptr<Definition<Class>>> localClasses;
    std::map<std::string, std::shared_ptr<Definition<Variable>>> members;
    std::vector<std::shared_ptr<Function>> methods;  // vector: functions can be overloaded
    std::map<std::string, std::shared_ptr<Definition<Typedef>>> typedefs;  // usually 'using A = B;'
    std::string docs;

    Class(const std::string& fullName, const std::string& superclass)
        : Type(kClass, fullName), super(superclass)
    {}

    bool isEmpty() const {
        return (enums.empty() && members.empty() && methods.empty() && localClasses.empty());
    }

    bool hasEnum(const std::string& name) const { return (enums.find(name) != enums.end()); }
    bool hasClass(const std::string& name) const { return (localClasses.find(name) != localClasses.end()); }
    bool hasMember(const std::string& name) const { return (members.find(name) != members.end()); }
    bool hasTypedef(const std::string& name) const { return (typedefs.find(name) != typedefs.end()); }

    void addEnum(const std::string& name, std::shared_ptr<Enum> o, Protection p)
    {
        assert(name.find(':') == std::string::npos);  // should be local name, not full namme
        if (o) {  // might be a forward declaration
            o->name = name;
        }
        enums[name] = std::make_shared<Definition<Enum>>(o, p);
    }

    void addClass(const std::string& name, std::shared_ptr<Class> o, Protection p)
    {
        assert(name.find(':') == std::string::npos);  // should be local name, not full namme
        if (o) {  // might be a forward declaration
            o->name = name;
        }
        localClasses[name] = std::make_shared<Definition<Class>>(o, p);
    }

    void addMember(const std::string& name, std::shared_ptr<Variable> o, Protection p)
    {
        assert(name.find(':') == std::string::npos);  // should be local name, not full namme
        if (o) {  // might be a forward declaration
            o->name = name;
        }
        members[name] = std::make_shared<Definition<Variable>>(o, p);
    }

    void addTypedef(const std::string& name, const std::string& targetType, Protection p)
    {
        assert(name.find(':') == std::string::npos);  // should be local name, not full namme
        auto t = std::make_shared<Typedef>(name, targetType);
        typedefs[name] = std::make_shared<Definition<Typedef>>(t, p);
    }

    std::vector<std::shared_ptr<Function>> calcSortedMethods(int protections)
    {
        std::vector<std::shared_ptr<Function>> out;
        out.reserve(out.size());
        for (auto &fIt : methods) {
            if (int(fIt->protection) & protections) {
                out.push_back(fIt);
            }
        }

        std::sort(out.begin(), out.end(),
                  [](const std::shared_ptr<Function>& x, const std::shared_ptr<Function>& y) {
            int xSortVal = x->hasQualifier("static") ? 1 : 100;
            int ySortVal = y->hasQualifier("static") ? 1 : 100;
            if (x->hasQualifier("constructor")) { xSortVal = 2; }
            if (y->hasQualifier("constructor")) { ySortVal = 2; }
            if (x->hasQualifier("destructor")) { xSortVal = 3; }
            if (y->hasQualifier("destructor")) { ySortVal = 3; }
            if (xSortVal != ySortVal) {
                return (xSortVal < ySortVal);
            } else {
                if (x->name != y->name) {
                    return (x->name < y->name);
                } else {
                    return (x->args.size() < y->args.size());
                }
            }
        });

        return out;
    }

    std::vector<std::shared_ptr<Variable>> calcSortedMembers(int protections)
    {
        std::vector<std::shared_ptr<Variable>> out;
        for (auto &memIt : members) {
            auto m = memIt.second;
            if (int(m->protection) & protections) {
                out.push_back(m->type);
            }
        }
        std::sort(out.begin(), out.end(),
                  [](const std::shared_ptr<Variable>& x, const std::shared_ptr<Variable>& y) {
            return x->name < y->name;
        });
        return out;
    }
};

//------------------------------ Tokenizer ------------------------------------
struct Token {
    enum Type { kError = 1, kEnd, kPrePocessor, kComment, kNumber, kString, kName };  // punct is a token with ascii val

    int type = Type::kError;
    std::string token;
    int lineNum;

    Token(int t, const std::string& tok, int ln) : type(t), token(tok), lineNum(ln) {}
};

class Stream
{
public:
    explicit Stream(const std::string& filename)
    {
        _filename = filename;
        _lineNum = 1;
        _data = nullptr;
        FILE *in = fopen(filename.c_str(), "r");
        if (in) {
            fseeko(in, 0, SEEK_END);
            auto len = ftello(in);
            fseeko(in, 0, SEEK_SET);
            _bytes = new char[len + 1];
            _data = _bytes;
            if (fread((void*)_data, len, 1, in) != 1) {
                std::cerr << "Error: failed reading '" << filename << "'" << std::endl;
            }
            _bytes[len] = '\0';
            _end = _bytes + len;
            fclose(in);
        } else {
            std::cerr << "Error: could not open '" << filename << "' for reading" << std::endl;
        }
    }

    ~Stream()
    {
        delete [] _bytes;
    }

    std::string filename() const { return _filename; }
    long lineNum() const { return _lineNum; }

    Token nextToken()
    {
        while (*_data != '\0') {
            assert(_data <= _end);
            skipWhitespace();
            assert(_data <= _end);
            if (*_data == '/' && *(_data+1) == '/') {
                return readCPPComment();
            } else if (*_data == '/' && *(_data+1) == '*') {
                return readCComment();
            } else if (*_data >= '0' && *_data <= '9') {
                return readNumber();
            } else if ((*_data >= 'A' && *_data <= 'Z') || (*_data >= 'a' && *_data <= 'z') || *_data == '_') {
                return readName();
            } else if ((*_data == '-' || *_data == '-') && (*(_data+1) >= '0' && *(_data+1) <= '9')) {
                return readNumber();
            } else if (*_data == '\"') {
                return readString();
            } else if (*_data == '\'') {
                return readChar();
            } else if (*_data == '#') {
                return readPreprocessor();
            } else if (*_data == '\0') {
                break;
            } else {
                auto punct = std::string(_data, 1);
                ++_data;
                return Token(int(punct[0]), punct, lineNum());
            }
        }
        return Token(Token::kEnd, "", -1);
    }

private:
    std::string _filename;
    long _lineNum;
    const char *_data;
    char *_bytes;
    const char *_end;


    void skipWhitespace()
    {
        while (*_data != '\0' && (*_data == ' ' || *_data == '\t' || *_data == '\r' || *_data == '\n')) {
            if (*_data == '\n') {
                ++_lineNum;
            }
            ++_data;
        }
    }

    Token readPreprocessor()
    {
        auto cmd = readName().token;
        skipWhitespace();
        auto start = _data;
        while (*_data != '\0' && *_data != '\n') {
            ++_data;
        }
        auto end = _data - 1;
        while (end > start && (*end == ' ' || *end == '\t' || *end == '\r')) {
            --end;
        }
        std::string line(start, end - start + 1);

        return Token(Token::Type::kPrePocessor, line, lineNum());
    }

    Token readCPPComment()
    {
        const auto *start = _data;
        while (*_data != '\0' && (*_data != '\n' || *_data != '\n')) {
            ++_data;
        }
        return Token(Token::kComment, std::string(start, _data - start), lineNum());
    }

    Token readCComment()
    {
        const auto *start = _data;
        while (*_data != '\0' && !(*_data == '*' && *(_data+1) == '/')) {
            ++_data;
        }
        if (*_data == '*') { ++_data; }
        if (*_data == '/') { ++_data; }
        return Token(Token::kComment, std::string(start, _data - start), lineNum());
    }

    Token readNumber()
    {
        const auto *start = _data;
        while (*_data != '\0'
               && ((*_data >= '0' && *_data <= '9') || *_data == '.' || *_data == '-' || *_data == 'e')) {
            ++_data;
        }
        return Token(Token::kNumber, std::string(start, _data - start), lineNum());
    }

    Token readChar()
    {
        const auto *start = _data;
        ++_data;
        while (*_data != '\0' && (*_data != '\'' || *(_data-1) == '\\')) {
            ++_data;
        }
        if (*_data == '\'') {
            ++_data;
        }
        return Token(Token::kNumber, std::string(start, _data - start), lineNum());
    }

    Token readString()
    {
        const auto *start = _data;
        ++_data;
        while (*_data != '\0' && (*_data != '"' || *(_data-1) == '\\')) {
            ++_data;
        }
        if (*_data == '"') {
            ++_data;
        }
        return Token(Token::kString, std::string(start, _data - start), lineNum());
    }

    Token readName()
    {
        const auto *start = _data;
        while (*_data != '\0'
               && ((*_data >= 'a' && *_data <= 'z') ||
                   (*_data >= 'A' && *_data <= 'Z') ||
                   (*_data >= '0' && *_data <= '9' && _data > start) ||
                   *_data == '_' ||
                   *_data == ':')) {
            ++_data;
        }
        // <name>::<name> is part of a name, but <name>: is a label
        while (_data > start && *(_data-1) == ':') {
            --_data;
        }
        return Token(Token::kName, std::string(start, _data - start), lineNum());
    }
};

//-----------------------------------------------------------------------------
class Names
{
public:
    const Type* get(const std::string& name) const
    {
        auto it = _names.find(name);
        if (it != _names.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    bool set(const std::string& name, std::shared_ptr<Type> type) // takes ownership of 'type'
    {
        bool alreadyHas = get(name);
        if (!alreadyHas || type != nullptr) {  // don't overwrite from a later forward-declaration
            _names[name] = type;
        }
        return alreadyHas;
    }

    void addNamespace(const std::string& ns) { _namespaces.insert(ns); }
    const std::set<std::string>& namespaces() const { return _namespaces; }

    const std::map<std::string, std::shared_ptr<Type>>& names() const { return _names; }

    const Type* lookupType(const std::string& symbol, const Class *clazz) const
    {
        auto *origClazz = clazz;

        while (clazz) {
            if (clazz->hasEnum(symbol)) {
                return clazz->enums.find(symbol)->second->type.get();
            } else if (clazz->hasClass(symbol)) {
                return clazz->localClasses.find(symbol)->second->type.get();
            } else if (clazz->name == symbol) {
                return clazz;
            }

            if (!clazz->super.empty()) {
                clazz = dynamic_cast<const Class*>(lookupFullName(clazz->super));
            } else {
                clazz = nullptr;
            }
        }
        clazz = origClazz;

        auto idx = clazz->fullName.rfind("::");
        while (idx != std::string::npos) {
            auto *t = lookupFullName(clazz->fullName.substr(0, idx) + "::" + symbol);
            if (t) {
                return t;
            }
            idx = clazz->fullName.rfind(idx - 1);
        }

        return nullptr;
    }

    const Type* lookupFullName(const std::string& fullName) const
    {
        std::vector<std::string> nsComponents;
        size_t startIdx = 0;
        for (size_t i = 0;  i < fullName.size();  ++i) {
            if (fullName[i] == ':') {
                nsComponents.push_back(fullName.substr(startIdx, i - startIdx));
                i += 2;
                startIdx = i;
            }
        }
        nsComponents.push_back(fullName.substr(startIdx));

        auto join = [](const std::vector<std::string>& ss, int startIdx, int endIdx) {
            std::string s;
            for (int i = startIdx;  i < endIdx;  ++i) {
                if (!s.empty()) { s += "::"; }
                s += ss[i];
            }
            return s;
        };
        size_t pivot = nsComponents.size();
        while (pivot > 0) {
            auto ns = join(nsComponents, 0, pivot);
            auto t = get(ns);
            if (t) {
                auto i = pivot;
                while (i < nsComponents.size()) {
                    if (t->type == kClass) {
                        std::string name = nsComponents[i];
                        auto *clazz = (const Class*)t;
                        if (clazz->hasEnum(name)) {
                            t = clazz->enums.find(name)->second->type.get();
                        } else if (clazz->hasClass(name)) {
                            t = clazz->localClasses.find(name)->second->type.get();
                        } /* else if (clazz->hasTypdef(name)) {
                        }*/
                    }
                    ++i;
                }
                if (i == nsComponents.size()) {
                    return t;
                }
            }
            --pivot;
        }
        return nullptr;
    }

    // Does not take into account any 'using namespace' directives (which should not be
    // in a header file anyway).
/*    std::shared_ptr<Type> lookup(const std::string& symbol, const std::string& currentNS,
                                 std::shared_ptr<Class> currentClass) const
    {
        if (currentClass) {
            if (currentClass->hasEnum(symbol)) {
                return currentClass->enums[symbol]->type;
            }
            if (currentClass->hasClass(symbol)) {
                return currentClass->localClasses[symbol]->type;
            }
            if (currentClass->hasMember(symbol)) {
                return currentClass->members[symbol]->type;
            }
            auto fIt = std::find_if(currentClass->methods.begin(), currentClass->methods.end(),
                            [&symbol](const std::shared_ptr<Function>& f) { return f->name == symbol; });
            if (fIt != currentClass->methods.end()) {
                return *fIt;
            }
        }

        auto globalIt = _names.find((currentNS.empty() ? "" : currentNS + "::") + symbol);
        if (globalIt != _names.end()) {
            return globalIt->second;
        }

        globalIt = _names.find(symbol);
        if (globalIt != _names.end()) {
            return globalIt->second;
        }

        return nullptr;
    } */

private:
    std::set<std::string> _namespaces;
    std::map<std::string, std::shared_ptr<Type>> _names;
} gGlobals;

class ParseContext
{
public:
    ParseContext withNamespace(const std::string& name)
    {
        ParseContext copy = *this;
        if (!copy._currentNamespace.empty()) {
            copy._currentNamespace += "::";
        }
        copy._currentNamespace += name;
        return copy;
    }

    ParseContext withClass(std::shared_ptr<Class> currentClass, Protection defaultProtection)
    {
        ParseContext copy = *this;
        if (!copy._currentClassFullName.empty()) {
            copy._currentClassFullName += "::";
        }
        copy._currentClassFullName += currentClass->name;
        copy._currentProtection = defaultProtection;
        copy._currentClass = currentClass;
        return copy;
    }

    bool isClass() const { return (_currentClass != nullptr); }

    std::shared_ptr<Class> currentClass() const { return _currentClass; }

    const std::string& currentClassFullName() const { return _currentClassFullName; }

    const std::string currentClassName() const { return _currentClass ? _currentClass->name : ""; }

    void addUsingNamespace(const std::string& ns)
        { _usingNamespaces.insert(ns); }
    void addTypedef(const std::string& name, const std::string& type)
        { _typedefs[name] = type; }

    void setProtection(Protection prot) { _currentProtection = prot; }
    Protection protection() const { return _currentProtection; }

    std::string getNextAnonymousClassName()
    {
        return "__anonymousClass" + std::to_string(++_anonymousStructs);
    }

    std::string calcFullName(const std::string& name)
    {
        std::string fullname;
        if (!_currentNamespace.empty()) {
            fullname += _currentNamespace + "::";
        }
        if (!_currentClassFullName.empty()) {
            fullname += _currentClassFullName + "::";
        }
        fullname += name;
        return fullname;
    }

/*    std::shared_ptr<Type> lookupType(const std::string& symbol) const
    {
        Class *clazz = _currentClass.get();
        while (clazz) {
            if (clazz->hasEnum(symbol)) {
                return clazz->enums[symbol]->type;
            } else if (clazz->hasClass(symbol)) {
                return clazz->localClasses[symbol]->type;
            } else if (clazz->name == symbol) {
                return _currentClass;
            }

            if (!clazz->super.empty()) {
                gGlobals.lookupFullName(clazz->fullName);
            } else {
                clazz = nullptr;
            }
        }
        return nullptr;
    } */

private:
    std::set<std::string> _usingNamespaces;
    std::map<std::string, std::string> _typedefs;
    std::string _currentNamespace;
    std::shared_ptr<Class> _currentClass;
    std::string _currentClassFullName;
    Protection _currentProtection = Protection::kPublic;
    int _anonymousStructs = 0;
};

std::string createError(const Stream& s, const std::string& msg)
{
    return s.filename() + ":" + std::to_string(s.lineNum()) + ": " + msg;
}

bool isSystemType(const std::string& type)
{
    if (kNativeTypes.find(type) != kNativeTypes.end()) {
        return true;
    }
    if (type.find("std::") == 0) {
        return true;
    }
    return false;
}

Type parseType(const std::vector<Token>& tokens, std::string *err)
{
    *err = "";

    int idx = 0;
    std::string name;
    bool isConst = false;

    if (tokens.size() == 1 && tokens[0].token == "void") {
        return Type(kNative, tokens[0].token);
    }

    if (tokens[idx].token == "const" || tokens[idx].token == "constexpr") {
        isConst = true;
        ++idx;
    }

    while (tokens[idx].token == "unsigned" || tokens[idx].token == "long") {  // "long long int"
        name += tokens[idx++].token + " ";
    }
    if (tokens[idx].type != Token::Type::kName) {
        *err = "invalid type name";
        return Type(kUnknown, "");
    } else {
        name += tokens[idx++].token;
    }

    if (tokens[idx].type == '<') {
        // TODO: handle templates better
        int n = 0;
        do {
            if (tokens[idx].type == '<') {
                ++n;
            } else if (tokens[idx].type == '>') {
                --n;
            }
            name += tokens[idx].token;
            ++idx;
        } while (idx < int(tokens.size()) && n > 0);
    }

    std::string mod;
    while (idx < tokens.size() &&
           (tokens[idx].type == '*' || tokens[idx].type == '&' || tokens[idx].token == "&&")) {
        mod += tokens[idx].token;
        ++idx;
    }

    if (idx < tokens.size() && tokens[idx].token == "const") {  // can't have constexpr here
        isConst = true;
        ++idx;
    }

    if (idx != int(tokens.size())) {
        *err = std::string("error parsing type, only ") + std::to_string(idx - 1) + " of " + std::to_string(int(tokens.size()) - 1) + " tokens used";
        return Type(kUnknown, "");
    }

    Type t(kUnknown, name);
    t.isConst = isConst;
    t.modifier = mod;
    return t;
}

std::string parseFunction(Stream& data, ParseContext& context, const std::string& docs,
                          std::vector<Token> tokens, Token *currentToken);
void parseVariableInitialValue(Stream &data, ParseContext& context, std::string *initialValue,
                               Token *currentToken);  // does not generate error

// Design note:
//   Although error messages are not necessary, since we only support parsing grammatically correct
//   files, copious error messages *really* helps with debugging! It usually immediately reveals
//   oversights in things the parser does not support, and often provides a convenient place to
//   put a breakpoint even when parsing lots of files.

std::string parse(Stream& data, const ParseContext& parentContext)
{
    ParseContext context = parentContext;
    std::vector<Token> tokenStack;
    std::string currentComment;
    int nAnonymousNamespaces = 0;

    Token tok = data.nextToken();
    while (tok.type != Token::Type::kEnd && tok.type != Token::Type::kError) {
//        std::cout << "[debug] " << data.filename() << ":" << data.lineNum() << ": token: '" << tok.token << "'" << std::endl;
//        if (data.lineNum() == 91) {
//            tok.type = tok.type;
//        }

        if (tok.type == Token::Type::kPrePocessor) {
            // do nothing for now
        } else if (tok.type == Token::Type::kComment) {
            if (tok.token.find("///") == 0) {
                if (!currentComment.empty()) {
                    currentComment += '\n';
                }
                currentComment += tok.token.substr(3);
            }
            // Ignore other kinds of comments. /* ... */ could be commenting out code
//            if (tok.token.find("/*") == 0) {
//                auto end = tok.token.size() - 2;
//                if (tok.token.rfind("*/") == tok.token.size() - 2) {
//                    end -= 2;
//                }
//                currentComment += tok.token.substr(2, end);
//            }
        } else if (tok.type == Token::Type::kName) {
            if (tok.token == "using") {
                auto name = data.nextToken();
                if (name.type != Token::Type::kName) {
                    return createError(data, "expected 'namespace' or <name> after 'using', got '" + name.token + "'");
                }
                tok = data.nextToken();
                if (tok.type == '=') {
                    tok = data.nextToken();
                    std::string type;
                    while (tok.type != Token::Type::kEnd && tok.type != ';') {
                        type += " " + tok.token;
                        tok = data.nextToken();
                    }
                    context.addTypedef(name.token, type);
                    if (context.isClass()) {
                        context.currentClass()->addTypedef(name.token, type, context.protection());
                    }
                } else if (tok.type == Token::Type::kName) {
                    context.addUsingNamespace(tok.token);
                    tok = data.nextToken();  // read the ';'
                } else {
                    return createError(data, "expected 'using namespace name ;' or 'using name1 = name2 ;'");
                }
                if (tok.type != ';') {
                    return createError(data, "expected ';' after 'using' statement");
                }
            } else if (tok.token == "namespace") {
                if (context.isClass()) {
                    return createError(data, "cannot create namespace within class/struct definition");
                }
                std::string name;
                tok = data.nextToken();
                if (tok.type == Token::Type::kName) {
                    name = tok.token;
                } else if (tok.type == '{') {
                    name = "anonymous_" + data.filename() + "_" + std::to_string(++nAnonymousNamespaces);
                } else {
                    return createError(data, "expected <name> or '{' after 'namespace', got '" + tok.token + "'");
                }
                tok = data.nextToken();
                if (tok.type == '{') {
                    gGlobals.addNamespace(context.calcFullName(name));
                    auto err = parse(data, context.withNamespace(name));
                    if (!err.empty()) {
                        return err;
                    }
                } else {
                    return createError(data, "expected '{' after 'namespace <name>'");
                }
            } else if (tok.token == "union") {
                return createError(data, "unions are not supported yet");
            } else if (tok.token == "class" || tok.token == "struct") {
                auto docs = currentComment;
                currentComment.clear();

                std::string super;
                bool isAnonymous = false;
                auto which = tok.token;
                auto name = data.nextToken();
                if (name.type == Token::Type::kName) {
                    tok = data.nextToken();
                } else if (name.type == '{') {
                    tok = name;
                    name.token = context.getNextAnonymousClassName();
                    isAnonymous = true;
                } else {
                    return createError(data, "expected <name> or '{' after '" + which + "'");
                }
                auto fullName = context.calcFullName(name.token);
                auto clazz = std::make_shared<Class>(fullName, "");
                if (tok.type == ';') {
                    clazz = std::shared_ptr<Class>();
                } else if (tok.type == ':') {
                    while (tok.type == ':' || tok.type == ',') {
                        auto prot = data.nextToken();
                        if (prot.type != Token::Type::kName) {
                            return createError(data, "expected public|protected|private after '" + which + " <name> :'");
                        }
                        if (prot.token == "protected" || prot.token == "private") {
                            std::cerr << data.filename() << ":" << data.lineNum() << ": only support 'public' inheritance" << std::endl;
                        }
                        tok = data.nextToken();
                        if (tok.type == Token::Type::kName) {
                            super = tok.token;
                        } else {
                            return createError(data, "expected <name> after '" + which + " <name> : <prot>'");
                        }
                        tok = data.nextToken();
                    }
                }
                if (tok.type == '{') {
                    auto err = parse(data,
                                     context.withClass(clazz,
                                                       which == "class" ? Protection::kPrivate
                                                                        : Protection::kPublic));
                    if (!err.empty()) {
                        return err;
                    }
                    tok = data.nextToken();
                    if (tok.type != ';' && !isAnonymous) {
                        return createError(data, "expected ';' after " + which + " definition");
                    }
                } else if (tok.type != ';') {
                    return createError(data, "expected '{' after '" + which + " <name>'");
                }
                if (clazz) {
                    clazz->super = super;
                    clazz->docs = docs;
                }

                if (context.isClass()) {
                    if (!context.currentClass()->hasClass(name.token))
                    {
                        context.currentClass()->addClass(name.token, clazz, context.protection());
                    } else {
                        return createError(data, which + " " + fullName + " is already defined");
                    }
                } else {
                    if (gGlobals.set(fullName, clazz)) {
                        // Could be a forward declaration (nullptr) after a defintion, otherwise is an error
                        if (clazz) {
                            return createError(data, "redefinition of " + which + " '" + fullName + "'");
                        }
                    }
                }

                // The only use for an anonymous struct is to use it as a variable
                if (isAnonymous && tok.type != ';') {
                    if (tok.type == Token::Type::kName) {
                        auto varName = tok;
                        std::string initialVal;  // unlikely, but is legal
                        tok = data.nextToken();
                        if (tok.type == ';') {
                            auto varType = Type(kClass, clazz->name);
                            auto fullName = context.calcFullName(varName.token);
                            auto varObj = std::make_shared<Variable>(fullName, varType);
                            varObj->defaultVal = initialVal;
                            if (context.isClass()) {
                                if (!context.currentClass()->hasMember(varName.token)) {
                                    context.currentClass()->addMember(varName.token, varObj,
                                                                      context.protection());
                                } else {
                                    return createError(data, "redefining member variable '" + fullName + "' with anonymous " + which);
                                }
                            } else {
                                if (gGlobals.set(fullName, varObj)) {
                                    return createError(data, "redefining global variable '" + fullName + "' with anonymous " + which);
                                }
                            }
                        } else if (tok.type == '=') {
                            parseVariableInitialValue(data, context, &initialVal, &tok);
                        } else {
                            return createError(data, "expected ';' after anonymous declaration '" + which + " { ... } <name>', got '" + tok.token + "'");
                        }
                    } else if (tok.type != ';') {
                        return createError(data, "unexpected '" + tok.token + "' after anonymous declaration; expected '" + which + " { ... } name;' or the useless but grammatically correct '" + which + " { ... };");
                    }
                }
            } else if (tok.token == "friend") {
                tok = data.nextToken();
                if (tok.token != "class") {
                    return createError(data, "expected 'class' after 'friend'");
                }
                auto tok = data.nextToken();
                if (tok.type == Token::Type::kName) {
                    // ignore friends
                } else {
                    return createError(data, "expected <name> after 'friend class'");
                }
                tok = data.nextToken();
                if (tok.type != ';') {
                    return createError(data, "expected ';' after 'friend class <name>'");
                }
            } else if (kProtectionNames.find(tok.token) != kProtectionNames.end()) {
                if (context.isClass()) {
                    if (tok.token == "public") {
                        context.setProtection(Protection::kPublic);
                    } else if (tok.token == "protected") {
                        context.setProtection(Protection::kProtected);
                    } else if (tok.token == "private") {
                        context.setProtection(Protection::kPrivate);
                    }
                } else {
                    return createError(data, "protection '" + tok.token + "' must be used within a class");
                }
                tok = data.nextToken();
                if (tok.type != ':') {
                    return createError(data, "class member protection label must end with ':'");
                }
            } else if (tok.token == "enum") {
                auto docs = currentComment;
                currentComment.clear();

                bool isEnumClass = false;
                tok = data.nextToken();
                if (tok.token == "class") {
                    isEnumClass = true;
                    tok = data.nextToken();
                }
                std::string name;
                if (tok.type == Token::Type::kName) {
                    name = tok.token;
                    tok = data.nextToken();
                }

                auto fullName = context.calcFullName(name);
                auto enumObj = std::make_shared<Enum>(fullName);
                if (isEnumClass) {
                    enumObj->qualifiers.insert("class");
                }
                enumObj->docs = docs;

                // Handle enum value type
                if (tok.type == ':') {
                    tok = data.nextToken();
                    std::string valueType;
                    while (tok.type == Token::Type::kName) {
                        if (!valueType.empty()) { valueType += " "; }  // e.g. 'unsigned', 'char'
                        valueType = tok.token;
                        tok = data.nextToken();
                    }
                }

                int lineNumOfLastValue = 0;
                if (tok.type == ';') {
                    // forward declaration, do nothing
                } else if (tok.type == '{') {
                    tok = data.nextToken();

                    std::string currentName;
                    while (tok.type != '}') {
                        if (tok.type == Token::Type::kName) {
                            currentName = tok.token;
                        } else {
                            return createError(data, "expected <name> at beginning of enum value");
                        }
                        tok = data.nextToken();

                        std::string val;
                        if (tok.type == '=') {
                            tok = data.nextToken();
                            while (tok.type != ',' && tok.type != '}' && tok.type != Token::Type::kEnd) {
                                val += tok.token;
                                tok = data.nextToken();
                            }
                        }

                        if (tok.type == ',' || tok.type == '}') {
                            enumObj->values.emplace_back(currentName, val, currentComment);
                            currentComment.clear();
                            lineNumOfLastValue = data.lineNum();
                        } else if (tok.type == Token::Type::kComment) {
                            // handled below; do nothing
                        } else {
                            return createError(data, "expected ',' or '}' after enum value");
                        }
                        if (tok.type == ',') {
                            tok = data.nextToken();
                        }
                        if (tok.type == Token::Type::kComment) {
                            bool isForLast = (tok.lineNum == lineNumOfLastValue);
                            std::string doc;
                            while (tok.type == Token::Type::kComment) {
                                if (!doc.empty()) {
                                    doc += "\n";
                                }
                                if (tok.token.find("///") == 0) {
                                    doc += tok.token.substr(3);
                                } else if (tok.token.find("//") == 0) {
                                    doc += tok.token.substr(2);
                                } else {
                                    doc += tok.token;
                                }
                                tok = data.nextToken();
                            }
                            if (isForLast) {
                                enumObj->values.back().docs = doc;
                            } else {
                                currentComment = doc;
                            }
                        }
                    }
                    tok = data.nextToken();
                } else {
                    return createError(data, "expected '{' or ';' after 'enum [class] [name]'");
                }

                if (tok.type != ';') {
                    return createError(data, "expected ';' after enum definition");
                }

                if (context.isClass()) {
                    if (!context.currentClass()->hasEnum(name)) {
                        context.currentClass()->addEnum(name, enumObj, context.protection());
                    } else {
                        return createError(data, "redefining enum '" + fullName + "'");
                    }
                } else {
                    if (gGlobals.set(fullName, enumObj)) {
                        return createError(data, "redefining enum '" + fullName + "'");
                    }
                }
            } else {
                tokenStack.push_back(tok);
            }
        } else if (tok.type == '(') {
            // We are either parsing a function and got to the open paren after the name,
            // or we are still parsing a template type with a paren in (e.g. std::function<void()>),
            // which might ultimately be the return type of a function or the type of a variable.
            int templateDepth = 0;
            for (auto &t : tokenStack) {
                if (t.type == '<') { ++templateDepth; }
                if (t.type == '>') { --templateDepth; }
            }

            if (templateDepth == 0) {
                auto docs = currentComment;
                currentComment.clear();

                auto err = parseFunction(data, context, docs, tokenStack, &tok);
                tokenStack.clear();  // must clear before err return!
                if (!err.empty()) {
                    return err;
                }
            } else {
                // continue parsing this type
            }
        } else if (tok.type == ';' || tok.type == '=') {
            if (tokenStack.empty() && tok.type == ';') {
                // this is a unnecessary semicolon (for example, as in 'void memberFn() { ... };'
                // do nothing
            } else if (tok.type == '=' &&
                       ((!tokenStack.empty() && tokenStack.back().token == "operator") ||
                        (tokenStack.size() >= 2 && tokenStack.back().type > ' ' && tokenStack[tokenStack.size() - 2].token == "operator"))) {
                // we have incompletely parsed 'operator=', 'operator==', 'operator!=', 'operator<=', etc.
                tokenStack.push_back(tok);
            } else {
                if (tokenStack.size() < 2) {
                    tokenStack.clear();
                    return createError(data, "too few tokens for variable definition");
                }
                // Handle any initial assignment to the variable/member
                std::string assignedVal;
                auto eqTokIt = std::find_if(tokenStack.begin(), tokenStack.end(),
                                            [](const Token& t) { return (t.type == '='); });
                if (eqTokIt != tokenStack.end()) {
                    for (auto it = eqTokIt + 1;  it != tokenStack.end();  ++it) {
                        assignedVal += it->token;
                    }
                    tokenStack.erase(eqTokIt, tokenStack.end());
                }

                if (tokenStack.back().type != Token::Type::kName) {
                    tokenStack.clear();
                    return createError(data, "invalid name '" + tokenStack.back().token + "' for class member definition");
                }
                auto memberName = tokenStack.back().token;
                tokenStack.pop_back();

                bool isStatic = false;
                if (tokenStack[0].token == "static") {
                    isStatic = true;
                    tokenStack.erase(tokenStack.begin());
                }
                std::string err = "";
                auto type = parseType(tokenStack, &err);
                tokenStack.clear();
                if (!err.empty()) {
                    return createError(data, err);
                }

                auto fullName = context.calcFullName(memberName);
                auto member = std::make_shared<Variable>(fullName, type);
                if (isStatic) {
                    member->qualifiers.insert("static");
                }
                member->defaultVal = assignedVal;

                // Handle any initial value that is assigned
                if (tok.type == '=') {
/*                    tok = data.nextToken();

                    std::string assignedVal;
                    while (tok.type != Token::Type::kEnd && tok.type != ';') {
                        assignedVal += tok.token;
                        tok = data.nextToken();
                    }
                    member->defaultVal = initialVal; */
                    parseVariableInitialValue(data, context, &member->defaultVal, &tok);
                }

                if (context.isClass()) {
                    if (!context.currentClass()->hasMember(memberName)) {
                        context.currentClass()->addMember(memberName, member, context.protection());
                    } else {
                        return createError(data, "redefinition of member variable '" + fullName + "'");
                    }
                } else {
                    if (!gGlobals.set(fullName, member)) {
                        return createError(data, "redefinition of variable '" + fullName + "'");
                    }
                }
            }
        } else if (tok.type == '}') {
            return "";
        } else {
            if (!tokenStack.empty() || // this could be a pointer/reference modifier or an operator
                (tok.type == '~' && tokenStack.empty()))  // could be destructor
            {
                tokenStack.push_back(tok);
            } else {
                return createError(data, "unexpected or unhandled token '" + tok.token + "'");
            }
        }
        tok = data.nextToken();
    }

    if (tok.type == Token::kEnd) {
        return "";
    } else if (tok.type == Token::kError) {
        return createError(data, "unprocessed error");
    } else {
        return createError(data, "unexpected token '" + tok.token + "' at end of file");
    }
}

std::string parseFunction(Stream& data, ParseContext& context,
                          const std::string& docs,
                          std::vector<Token> tokenStack, /* copies intentionally */
                          Token *currentToken)
{
    Token tok = *currentToken;

    if (tokenStack.empty()) {
        return createError(data, "function declaration with no name");
    }

    // Determine function name
    Token name = tokenStack.back();  // copies
    tokenStack.pop_back();
    if (name.token == "operator") {  // this is operator(), but we stopped at the open paren
        tok = data.nextToken();
        if (tok.type != ')') {
            return createError(data, "expected definition of 'operator()', expected token ')' but got '" + tok.token + "'");
        }
        tok = data.nextToken();
        if (tok.type != ')') {
            return createError(data, "expected definition of 'operator()', expected token '(' but got '" + tok.token + "'");
        }
    } else if (name.type == Token::Type::kName) {
        // name was assigned above; do nothing
    } else {
        auto opIt = std::find_if(tokenStack.begin(), tokenStack.end(),
                                 [](const Token& t) { return (t.token == "operator"); });
        if (opIt != tokenStack.end()) {
            auto last = name.token;
            name = *opIt;
            for (auto it = opIt + 1;  it != tokenStack.end();  ++it) {
                name.token += it->token;
            }
            name.token += last;  // this got popped off the token stack on the assumption it was the name
            tokenStack.erase(opIt, tokenStack.end());
        } else {
            return createError(data, "function name '" + name.token + "' is not a valid name");
        }
    }
    if (!tokenStack.empty() && tokenStack.back().token == "~") {  // destructor
        name.token = "~" + name.token;
        tokenStack.pop_back();
    }

    auto fullName = context.calcFullName(name.token);
    auto func = std::make_shared<Function>(fullName);
    func->protection = context.protection();
    func->docs = docs;

    if (!tokenStack.empty() && tokenStack.front().token == "static") {
        func->qualifiers.insert(tokenStack.front().token);
        tokenStack.erase(tokenStack.begin());
    }
    if (!tokenStack.empty() && tokenStack.front().token == "virtual") {
        func->qualifiers.insert(tokenStack.front().token);
        tokenStack.erase(tokenStack.begin());
    }
    if (!tokenStack.empty() && tokenStack.front().token == "explicit") {
        func->qualifiers.insert(tokenStack.front().token);
        tokenStack.erase(tokenStack.begin());
    }

    if (tokenStack.size() == 0) {
        if (name.token == context.currentClassName()) {  // constructor
            func->qualifiers.insert("constructor");
            func->returnType = Type(kClass, context.currentClassFullName());
        } else if (name.token == "~" + context.currentClassName()) {  // destructor
            func->returnType = Type(kNative, "void");
            func->qualifiers.insert("destructor");
        } else {
            return createError(data, "function has no return type");
        }
    } else {
        std::string err;
        func->returnType = parseType(tokenStack, &err);
        if (!err.empty()) {
            return createError(data, err);
        }
    }

    std::vector<Token> argTokens;
    tok = data.nextToken();
    int templateDepth = 0;
    while (tok.type != Token::Type::kEnd) {
        if (templateDepth == 0 && (tok.type == ',' || tok.type == ')' || tok.type == '=')) {
            std::string defaultVal;
            if (tok.type == '=') {
                int parenDepth = 0;
                tok = data.nextToken();
                while (tok.type != Token::Type::kEnd
                       && ((tok.type != ',' && tok.type != ')') || parenDepth > 0)) {
                    defaultVal += tok.token;
                    if (tok.type == '(' || tok.type == '{') { ++parenDepth; }
                    if (tok.type == ')' || tok.type == '}') { --parenDepth; }
                    tok = data.nextToken();
                }
            }

            bool hasNoArgs = (tok.type == ')' && func->args.empty () && argTokens.empty());
            if (!hasNoArgs) {
                if (argTokens.size() == 0) {
                    return createError(data, "too few tokens for function argument #" + std::to_string(argTokens.size()));
                }

                // It is possible (although unusual) for the argument to not have a name
                // (avoids compile warnings about unused parameters). Since we are not compiling
                // header files, we cannot know for certain if a user class/struct/enum is passed,
                // but since classes and structs are usually passed by ptr, ref, or move-ref,
                // this should catch most cases.
                std::string argName;
                if (argTokens.size() >= 2 && argTokens.back().type == Token::Type::kName
                    && !isSystemType(argTokens.back().token))
                {
                    argName = argTokens.back().token;
                    argTokens.pop_back();
                }

                std::string err;
                auto t = parseType(argTokens, &err);
                if (err.empty()) {
                    func->args.push_back(Function::Arg{t, argName, defaultVal});
                    argTokens.clear();
                } else {
                    return createError(data, err);
                }
            }
        } else if (tok.type == '<') {
            argTokens.push_back(tok);
            ++templateDepth;
        } else if (tok.type == '>') {
            argTokens.push_back(tok);
            --templateDepth;
        } else {
            argTokens.push_back(tok);
        }
        if (tok.type == ')' && templateDepth <= 0) { // e.g. 'void f(std::function<void(int)> callback);'
            break;
        }
        tok = data.nextToken();
    }

    do {
        tok = data.nextToken();
        if (tok.token == "const") {
            func->isConst = true;
        } else if (tok.token == "override") {
            func->qualifiers.insert("virtual");
            func->qualifiers.insert("override");
        } else if (tok.type == '=') {
            tok = data.nextToken();
            if (tok.token == "0") {
                func->qualifiers.insert("pure");
            } else if (tok.token == "delete") {
                func->qualifiers.insert("delete");
            } else {
                return createError(data, "expected '= 0' or '= delete' after function definition, got '= '" + tok.token + "'");
            }
        } else if (tok.type == ':' || tok.type == '{') {  // constructor or inline definition (ignore)
            int braceDepth = 0;
            while (tok.type != '}' || braceDepth > 1) {  // will be depth of 1 b/c closing definition
                if      (tok.type == '{') { ++braceDepth; }
                else if (tok.type == '}') { --braceDepth; }
                tok = data.nextToken();
            }
            break;  // no ';' after inline definition
        } else if (tok.type == ';') {
            // end of definition, not an error; do nothing
        } else {
            return createError(data, "unexpected token '" + tok.token + "' after function definition");
        }
    } while (tok.type != ';' && tok.type != Token::Type::kEnd);

    if (context.isClass()) {
        context.currentClass()->methods.push_back(func);
    } else {
        gGlobals.set(fullName, func);
    }

    *currentToken = tok;
    return "";
}

void parseVariableInitialValue(Stream &data, ParseContext& context, std::string *initialValue,
                               Token *currentToken)
{
    auto tok = *currentToken;
    assert(tok.type == '=');
    tok = data.nextToken();

    *initialValue = "";
    while (tok.type != Token::Type::kEnd && tok.type != ';') {
        *initialValue += tok.token;
        tok = data.nextToken();
    }

    *currentToken = tok;
}
//-----------------------------------------------------------------------------
class Generator
{
public:
    virtual ~Generator() {}

    virtual std::string generate() = 0;
};

class GenerateDocs : public Generator
{
public:
    GenerateDocs(const std::string& outputDir)
        : mOutputDir(outputDir)
        , mCSSFile("style.css")
    {
    }

    std::string generate() override
    {
        auto err = generateCSS(mCSSFile);
        if (!err.empty()) {
            return err;
        }

        err = generateIndex("index.html");
        if (!err.empty()) {
            return err;
        }

        err = generateDocs();
        if (!err.empty()) {
            return err;
        }
        
        return "";
    }

private:
    std::string generateCSS(const std::string& filename)
    {
        std::vector<std::string> lines = {
            "body {",
            "  font-family: \"Georgia\";"
            "}",
            "a {",
            "  color: #000044;",
            "  text-decoration: none;",
            "}",
            "a:hover {",
            "  text-decoration: underline;",
            "}",
            "",
            ".content {",
            "  min-width: 20em;",
            "  max-width: 60em;",
            "  margin-left: auto;",
            "  margin-right: auto;",
            "}",
            ".section {",
            "  font-weight: bold;",
            "}",
            ".indexList {",
            "  columns: 3;",
            "  list-style-type: none;",
            "}",
            ".protectionLevel {",
            "  margin-left: 2em;",
            "}",
            ".enum {",
            "  margin-left: 2em;"
            "}",
            ".enumVals {",
            "  margin-left: 4em;"
            "}",
            ".typeCell {",
            "  vertical-align: top;",
            "  text-align: right;",
            "}",
            ".name {",
            "  font-weight: bold",
            "}",
            ".classDetails {",
            "}",
            ".detailsDef {",
            "  margin-top: 1em;",
            "  margin-left: 2em;",
            "  border-bottom: 1px solid #cccccc;",
            "  font-size: 110%",
            "}",
            ".details {",
            "  margin-left: 4em;",
            "  font-size: 100%",
            "}",
        };
        return writeFile(mOutputDir + "/" + filename, lines);
    }

    std::string generateIndex(const std::string& filename)
    {
        std::vector<std::string> lines = {
            "<!DOCTYPE html>",
            "<head>",
            "  <title>Symbol index</title>",
            "  <link href=\"" + mCSSFile + "\" rel=\"stylesheet\" />",
            "</head>",
            "<body>",
            "<div class=\"content\">",
        };

        std::vector<std::string> namespaces;
        namespaces.reserve(gGlobals.namespaces().size() + 1);
        namespaces.push_back("");  // global namespace
        namespaces.insert(namespaces.end(), gGlobals.namespaces().begin(), gGlobals.namespaces().end());
        std::sort(namespaces.begin(), namespaces.end());

        for (auto &ns : namespaces) {
            std::vector<std::string> symbols;
            for (auto &n : gGlobals.names()) {
                const auto nsPrefix = ns + "::";
                if ((ns.empty() || n.first.find(nsPrefix) == 0)
                    && n.first.find("::", nsPrefix.size()) == std::string::npos
                    && n.second != nullptr /* not just a forward declaration */)
                {
                    symbols.push_back(n.first.substr(nsPrefix.size()));
                }
            }
            std::sort(symbols.begin(), symbols.end());

            if (!symbols.empty()) {
                lines.push_back("<h3>" + (ns.empty() ? "global" : ns) + "</h3>");
                lines.push_back("<ul class=\"indexList\">");
                for (auto &s : symbols) {
                    auto fullName = ((ns.empty()) ? "" : ns + "::") + s;
                    auto href = calcHref(fullName);
                    lines.push_back("<li><a href=\"" + href + "\">" + s + "</a></li>");
                }
                lines.push_back("</ul>");
            }
        }

        lines.push_back("</div>");
        lines.push_back("</body>");
        lines.push_back("</html>");
        return writeFile(mOutputDir + "/" + filename, lines);
    }

    std::string generateDocs()
    {
        std::vector<std::string> names;
        for (auto &n : gGlobals.names()) {
            if (n.second != nullptr) {
                names.push_back(n.first);
            }
        }
        std::sort(names.begin(), names.end());  // ensures directories get created in descending tree order

        std::vector<std::string> toplevelDefinitions;

        for (auto &fullName : names) {
            auto *obj = gGlobals.get(fullName);
            if (!obj) {
                continue;
            }
            if (obj->type == kClass) {
                auto href = calcHref(fullName);
                std::string cssRelativeDir;
                {
                    auto idx = href.find("/");
                    while (idx != std::string::npos) {
                        cssRelativeDir += "../";
                        idx = href.find("/", idx + 1);
                    }
                }
                Class *clazz = (Class*)obj;
                std::vector<std::string> lines = {
                    "<!DOCTYPE html>",
                    "<head>",
                    "  <title>" + fullName + "</title>",
                    "  <link href=\"" + cssRelativeDir + mCSSFile + "\" rel=\"stylesheet\" />",
                    "</head>",
                    "<body>",
                    "<div class=\"content\">",
                    "  <h2>" + fullName + "</h2>"
                };

                if (!clazz->super.empty()) {
                    lines.push_back("<p>");
                    lines.push_back("Inherits from: " + clazz->super);
                    lines.push_back("</p>");
                }

                if (!clazz->docs.empty()) {
                    lines.push_back("<p class=\"classDetails\">");
                    lines.push_back(calcHtmlDocString(clazz->docs, clazz));
                    lines.push_back("</p>");
                }

                auto addDefinitions = [this, clazz, &lines](Protection protection) {
                    std::vector<std::shared_ptr<Class::Definition<Enum>>> enums;
                    for (auto &enumIt : clazz->enums) {
                        auto en = enumIt.second;
                        if (en->protection == protection) {
                            enums.push_back(en);
                        }
                    }
                    std::vector<std::shared_ptr<Class::Definition<Class>>> classes;
/*                   for (auto &c : clazz->localClasses) {
                        if (c.protection == Protection::kPublic) {
                            appendLines(&lines, classDocs(&c));
                        }
                    } */
                    // TODO: include 'using XYZ = abc;' in the types section (see UID in Accessibility.h)

                    auto members = clazz->calcSortedMembers(int(protection));
                    auto methods = clazz->calcSortedMethods(int(protection));

                    if (enums.size() + classes.size() + members.size() + methods.size() > 0) {
                        std::string prot = "Public";
                        if (protection == Protection::kProtected) {
                            prot = "Protected";
                        } else if (protection == Protection::kPrivate) {
                            prot = "Private";
                        }
                        lines.push_back("<h3>" + prot + "</h3>");
                        lines.push_back("<div class=\"protectionLevel\">");

                        bool hasTypes = (enums.size() + classes.size() > 0);
                        if (hasTypes) {
                            lines.push_back("<div class=\"section\">Types</div>");
                            for (auto &en : enums) {
                                appendLines(&lines, enumShortDocs(clazz, en->type.get()));
                            }
                        }

                        if (members.size() + methods.size() > 0) {
                            if (hasTypes) {
                                lines.push_back("<br>");
                            }
                            lines.push_back("<table colspacing=\"0\" rowspacing=\"0\">");
                            if (!methods.empty()) {
                                lines.push_back("<tr><td class=\"section\" colspan=\"2\">Methods</td></tr>");
                                for (auto &f : methods) {
                                    appendLines(&lines, functionShortDocs(clazz, f.get()));
                                }
                            }
                            if (!members.empty()) {
                                lines.push_back("<tr></tr>");
                                lines.push_back("<tr><td class=\"section\" colspan=\"2\">Members</td></tr>");
                                for (auto &m : members) {
                                    appendLines(&lines, memberShortDocs(clazz, m.get()));
                                }
                            }
                            lines.push_back("</table>");
                        }

                        lines.push_back("</div>");
                    }
                };

                addDefinitions(Protection::kPublic);
                addDefinitions(Protection::kProtected);

                lines.push_back("<h3>Details</h3>");

                auto methods = clazz->calcSortedMethods(int(Protection::kPublic) |
                                                        int(Protection::kProtected));
                for (auto &f : methods) {
                    appendLines(&lines, functionDocs(clazz, f.get()));
                }

                lines.push_back("</div>");
                lines.push_back("</body>");
                lines.push_back("</html>");

                auto err = writeFile(mOutputDir + "/" + href, lines);
                if (!err.empty()) {
                    return err;
                }
            } else {
                toplevelDefinitions.push_back(fullName);
            }
        }

        return "";
    }

    std::vector<std::string> enumShortDocs(const Class *clazz, const Enum *obj)
    {
        std::string qual;
        if (obj->hasQualifier("class")) { qual += "class "; }
        std::string datatype;
        if (!obj->valueType.empty()) {
            datatype = " : " + obj->valueType;
        }

        std::vector<std::string> lines;
        lines.push_back("<div class=\"enum\">enum " + qual +
                        "<span class=\"name\">" + obj->name + "</span>" +
                         datatype + "</div>");
        lines.push_back("<div class=\"enumVals\">");
        lines.push_back("  <table colspacing=\"0\" rowspacing=\"0\">");
        for (auto &val : obj->values) {
            lines.push_back("    <tr><td>" + val.name + "</td>" +
                            (val.value.empty() ? "" : "<td>= " + val.value + "</td>"));
        }
        lines.push_back("  </table>");
        lines.push_back("</div>");
        return lines;
    }

    std::vector<std::string> memberShortDocs(const Class *clazz, const Variable *member)
    {
        std::vector<std::string> lines;
        std::string qual;
        if (member->hasQualifier("static")) { qual += "static "; }

        lines.push_back("  <tr><td class=\"typeCell\">" + qual + typeString(&member->type, clazz, Linking::kNo) + "</td><td><span class=\"name\">" + member->name + "</span>" + (member->defaultVal.empty() ? "" : " = " + member->defaultVal) + "</td><tr>");
        return lines;
    }

    std::vector<std::string> functionShortDocs(const Class *clazz, const Function *func)
    {
        std::vector<std::string> lines;
        std::string qual;
        if (func->hasQualifier("static")) { qual += "static "; }
        if (func->hasQualifier("virtual")) { qual += "virtual "; }
        if (func->hasQualifier("explicit")) { qual += "explicit "; }

        std::string returnType;
        if (!func->hasQualifier("constructor") && !func->hasQualifier("destructor")) {
            returnType = typeString(&func->returnType, clazz, Linking::kNo);
        }

        lines.push_back("  <tr><td class=\"typeCell\">" + qual + returnType + "</td><td><a class=\"name\" href=\"#" + func->identifier() + "\">" + func->name + "</a>(");
        lines.back() += functionArgsString(func->args, clazz, Linking::kNo);
        lines.back() += ")";
        if (func->isConst) {
            lines.back() += " const";
        }
        if (func->hasQualifier("override")) {
            lines.back() += " override";
        }
        if (func->hasQualifier("pure")) {
            lines.back() += " = 0";
        }
        if (func->hasQualifier("delete")) {
            lines.back() += " = delete";
        }
        lines.push_back("</td></tr>");
        return lines;
    }

    std::vector<std::string> functionDocs(const Class *clazz, const Function *func)
    {
        std::string returnType;
        if (!func->hasQualifier("constructor") && !func->hasQualifier("destructor")) {
            returnType = typeString(&func->returnType, clazz, Linking::kYes);
        }

        auto name = func->name;
//        if (!className.empty()) {
//            name = className + "::" + name;
//        }
        auto argsStr = "(" + functionArgsString(func->args, clazz, Linking::kYes) + ")";
        if (func->isConst) {
            argsStr += " const";
        }

        std::string quals;
        auto addQualifier = [&quals, func /* ptr is ok, not shared_ptr */](const std::string& q, const std::string& betterName = "") {
            if (func->hasQualifier(q)) {
                if (!quals.empty()) { quals += ", "; }
                if (betterName.empty()) {
                    quals += q;
                } else {
                    quals += betterName;
                }
                return true;
            }
            return false;
        };
        addQualifier("static");
        if (!addQualifier("override")) {
            if (!addQualifier("pure", "pure virtual")) {
                addQualifier("virtual");
            }
        }
        if (addQualifier("constructor")) {
            addQualifier("explicit");
        }
        addQualifier("destructor");
        addQualifier("delete", "deleted");
        if (!quals.empty()) {
            quals = " [" + quals + "]";
        }

        std::vector<std::string> lines;
        lines.push_back("<div id=\"" + func->identifier() + "\" class=\"detailsDef\">" + returnType +
                        (returnType.empty() ? "" : "<br>") +
                        "<span class=\"name\">" + name + "</span>" + argsStr + quals + "</div>");
        if (!func->docs.empty()) {
            lines.push_back("<div class=\"details\">" + calcHtmlDocString(func->docs, clazz) + "</div>");
        }
        return lines;
    }

    std::string calcHtmlDocString(const std::string& docStr, const Class *clazzContext)
    {
        std::string html = docStr;

        bool inBackticks = false;
        bool inList = false;
        int xPos = -1; // the docs have a space after the comment, so xPos = 0 is one space after the newline
        for (std::string::size_type i = 0;  i < html.size();  ++i) {
            if (html[i] == '`') {
                if (inBackticks) {
                    html.replace(i, 1, "</tt>");
                    inBackticks = false;
                    i += 5;
                } else {
                    html.replace(i, 1, "<tt>");
                    inBackticks = true;
                    i += 4;
                }
            } else if (html[i] == '-' && xPos <= 0 && i < html.size() - 1 && html[i+1] == ' ') {
                if (!inList) {
                    inList = true;
                    html.replace(i, 2, "<ul><li>");
                    i += 8;
                } else {
                    if (inBackticks) {
                        html.insert(i, "</tt>");
                        i += 5;
                    }
                    html.replace(i, 2, "</li><li>");
                    i += 9;
                }
            } else if (html[i] == '\n') {
                xPos = -1;
                if (i >= html.size() - 1 && !(html[i+1] == '-' || std::isspace(html[i+1]))) {
                    inList = false;
                    if (inBackticks) {
                        html.insert(i, "</tt>");
                        i += 5;
                    }
                    html.insert(i, "</li></ul>");
                    i += 10;
                }
            }

            if (html[i] != '\n') {
                ++xPos;
            }
        }
        if (inBackticks) {
            html.append("</tt>");
        }

        return html;
    }

    enum class Linking { kNo = 0, kYes = 1 };

    std::string functionArgsString(const std::vector<Function::Arg>& args, const Class *clazz,
                                   Linking linking)
    {
        std::string argsStr;
        for (size_t i = 0;  i < args.size();  ++i) {
            auto &arg = args[i];
            auto s = typeString(&arg.type, clazz, linking) + "&nbsp;" + arg.name +
                     (arg.defaultVal.empty() ? "" : "&nbsp;=&nbsp;" + arg.defaultVal);
            if (i == 0) {
                argsStr += s;
            } else {
                argsStr += ", ";
                argsStr += s;
            }
        }
        return argsStr;
    }

    std::string typeString(const Type *type, const Class *clazz, Linking linking)
    {
        assert(type->type != kFunction || type->type != kVariable);

        //--- debug
//        if (type->fullName == "OSMenubar") {
//            linking = linking;
//        }
        //---

        auto isNative = isSystemType(type->fullName);

        std::vector<std::string> namespaceTree;
        std::string name;
        std::string href;
        if (isNative) {
            name = type->fullName;
            href = "";
        } else if (!clazz) {
            name = type->fullName;
            href = calcHref(name);
        // Types are generally relative (unqualified), so look up in class first
        } else if (clazz->hasEnum(type->fullName) || clazz->hasClass(type->fullName) ||
                   clazz->hasTypedef(type->fullName)) {
            name = type->fullName;
            href = "#" + type->identifier();
        } else if (type->fullName == clazz->name) {
            name = type->fullName;
            href = "";  // same class, don't link to this page
        // Type is not in the tree, treat as fully qualified
        } else {
            auto *actualType = gGlobals.lookupType(type->fullName, clazz);
            std::string typeFullName;
            if (actualType) {
                typeFullName = actualType->fullName;
            } else {
                typeFullName = type->fullName;
            }

            size_t idx = 0;
            while (idx < typeFullName.size() && idx < clazz->fullName.size()
                   && typeFullName[idx] == clazz->fullName[idx])
            {
                ++idx;
            }

            if (idx == typeFullName.size() && idx == clazz->fullName.size()) {
                name = type->name;  // types are identical
            } else {  // type.size() < clazz (if =, would hit above)
                if (idx < clazz->fullName.size() && clazz->fullName[idx] == ':') {
                    name = type->name;  // type is parent of clazz (if equal, would have hit the above check)
                } else {
                    auto i = typeFullName.rfind(':', idx);
                    if (i == std::string::npos) {
                        name = typeFullName;
                    } else {
                        name = typeFullName.substr(i + 1);
                    }
                }
            }

            if (actualType) {
                href = calcHref(typeFullName);
            }
            assert(!name.empty());
        }

        // Escape name if it has '<' or '>', and substitute ' ' with non-breaking space.
        // Do this from the back so that the index is the same after replacing.
        for (int i = int(name.size()) - 1;  i >= 0;  --i) {  // 0 - 1 is > 0 with size_t!
            switch (name[i]) {
                case '<':  name.replace(i, 1, "&lt;"); break;
                case '>':  name.replace(i, 1, "&gt;"); break;
                case ' ':  name.replace(i, 1, "&nbsp;"); break;
            }
        }

        std::string ts = (type->isConst ? "const&nbsp;" : "");
        if (linking == Linking::kYes) {
            if (!href.empty()) {
                ts += "<a href=\"" + href + "\">" + name + "</a>";
            } else {
                ts += name;
            }
        } else {
            ts += name;
        }
        ts += type->modifier;
        return ts;
    }

    void appendLines(std::vector<std::string> *lines, const std::vector<std::string>& toAppend)
    {
        lines->insert(lines->end(), toAppend.begin(), toAppend.end());
    }

    std::string calcHref(std::string name /* copy */)
    {
        bool isNamedIndex = (name == "index");

        auto idx = name.find("::");
        while (idx != std::string::npos) {
            name.replace(idx, 2, "/");
            idx = name.find("::");
        }
        if (!isNamedIndex) {
            return name + ".html";
        } else {
            return "__index.html";  // avoid clashes with index.html if namespace has symbol is named "index"
        }
    }

    std::string writeFile(const std::string& path, const std::vector<std::string>& lines)
    {
        auto parentDir = uitk::Directory(uitk::File(path).parentPath());
        if (!parentDir.path().empty() && !parentDir.exists()) {
            if (parentDir.mkdir() != uitk::IOError::kNone) {
                return "Could not create directory '" + parentDir.path() + "'";
            }
        }

        FILE *out = fopen(path.c_str(), "wb"); // write in binary so Windows doesn't use \r\n
        if (out) {
            for (auto &line : lines) {
                auto len = line.size();
                if (len > 0) {
                    if (fwrite(line.c_str(), len, 1, out) != 1) {
                        fclose(out);
                        return "Incomplete write to '" + path + "'";
                    }
                }
                if (fwrite("\n", 1, 1, out) != 1) {
                    fclose(out);
                    return "Incomplete write to '" + path + "'";
                }
            }
            fclose(out);
            return "";
        }
        return "Could not open '" + path + "' for writing'";
    }

private:
    std::string mOutputDir;
    std::string mCSSFile;
};

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int nErrors = 0;
    std::string path(argv[1]);

    std::vector<std::string> files;
    if (uitk::Directory(path).isDir()) {
        uitk::IOError::Error err;
        auto entries = uitk::Directory(path).entries(&err);
        if (err) {
            std::cerr << "Error reading directory '" << path << "'" << std::endl;
            return 1;
        }
        files.reserve(entries.size());
        for (auto &e : entries) {
            auto ext = e.extension();
            if (e.isFile && (ext == "h" || ext == "hpp" || ext == "H")) {
                files.push_back(path + "/" + e.name);
            }
        }
    } else {
        files.push_back(argv[1]);
    }

    for (auto &f : files) {
        Stream input(f);
        auto err = parse(input, ParseContext());
        if (!err.empty()) {
            std::cerr << "Error: " << err << std::endl;
            ++nErrors;
        }
    }

    GenerateDocs docs("/tmp/docs");
    auto err = docs.generate();
    if (!err.empty()) {
        std::cerr << "Error generating: " << err << std::endl;
        ++nErrors;
    }

    return nErrors;
}
