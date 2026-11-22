#ifndef VAULT_JSON_DOCUMENT_H
#define VAULT_JSON_DOCUMENT_H

#include <string>
#include "backend/OperationEvent.h"

// Deliberately narrow JSON object reader: strings, numbers and integer arrays.
class JsonDocument
{
private:
    struct Field
    {
        std::string name;
        std::string kind;
        std::string text;
        double number;
        IntBuffer array;
        Field();
    };
    Field* fields;
    int count;
    const Field* find(const std::string& name) const;
    static void whitespace(const std::string& text, int& position);
    static bool quoted(const std::string& text, int& position, std::string& value);
    static bool numeric(const std::string& text, int& position, double& value);
    bool parse(const std::string& text);
    JsonDocument(const JsonDocument&) = delete;
    JsonDocument& operator=(const JsonDocument&) = delete;

public:
    JsonDocument();
    ~JsonDocument();
    bool load(const std::string& path);
    bool string(const std::string& name, std::string& value) const;
    bool number(const std::string& name, double& value) const;
    bool integer(const std::string& name, int& value) const;
    bool array(const std::string& name, IntBuffer& value) const;
    bool only(const std::string& names) const;
};

#endif
