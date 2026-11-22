#include "middleware/JsonDocument.h"
#include <fstream>
#include <cstdlib>
#include <cmath>
#include <climits>

JsonDocument::Field::Field() : number(0)
{
}

JsonDocument::JsonDocument() : fields(NULL), count(0)
{
}

JsonDocument::~JsonDocument()
{
    delete[] fields;
}

void JsonDocument::whitespace(const std::string& text, int& position)
{
    while (position < static_cast<int>(text.size()) &&
           (text[position] == ' ' || text[position] == '\n' || text[position] == '\r' || text[position] == '\t'))
    {
        position++;
    }
}

bool JsonDocument::quoted(const std::string& text, int& position, std::string& value)
{
    value.clear();
    if (position >= static_cast<int>(text.size()) || text[position++] != '"')
    {
        return false;
    }
    while (position < static_cast<int>(text.size()))
    {
        char letter = text[position++];
        if (letter == '"')
        {
            return true;
        }
        if (static_cast<unsigned char>(letter) < 32)
        {
            return false;
        }
        if (letter == '\\')
        {
            if (position >= static_cast<int>(text.size()))
            {
                return false;
            }
            letter = text[position++];
            if (letter != '"' && letter != '\\' && letter != '/')
            {
                return false;
            }
        }
        value += letter;
    }
    return false;
}

bool JsonDocument::numeric(const std::string& text, int& position, double& value)
{
    int start = position;
    int length = static_cast<int>(text.size());
    if (position < length && text[position] == '-')
    {
        position++;
    }
    if (position >= length || text[position] < '0' || text[position] > '9')
    {
        return false;
    }
    if (text[position] == '0')
    {
        position++;
        if (position < length && text[position] >= '0' && text[position] <= '9')
        {
            return false;
        }
    }
    else
    {
        while (position < length && text[position] >= '0' && text[position] <= '9')
        {
            position++;
        }
    }
    if (position < length && text[position] == '.')
    {
        position++;
        int firstDigit = position;
        while (position < length && text[position] >= '0' && text[position] <= '9')
        {
            position++;
        }
        if (position == firstDigit)
        {
            return false;
        }
    }
    if (position < length && (text[position] == 'e' || text[position] == 'E'))
    {
        position++;
        if (position < length && (text[position] == '+' || text[position] == '-'))
        {
            position++;
        }
        int firstDigit = position;
        while (position < length && text[position] >= '0' && text[position] <= '9')
        {
            position++;
        }
        if (position == firstDigit)
        {
            return false;
        }
    }
    value = std::strtod(text.substr(start, position - start).c_str(), NULL);
    return std::isfinite(value);
}

bool JsonDocument::parse(const std::string& text)
{
    delete[] fields;
    fields = NULL;
    fields = new Field[64];
    count = 0;
    int position = 0;
    whitespace(text, position);
    if (position >= static_cast<int>(text.size()) || text[position++] != '{')
    {
        return false;
    }
    whitespace(text, position);
    while (position < static_cast<int>(text.size()) && text[position] != '}')
    {
        if (count == 64)
        {
            return false;
        }
        Field& field = fields[count];
        if (!quoted(text, position, field.name) || find(field.name) != NULL)
        {
            return false;
        }
        whitespace(text, position);
        if (position >= static_cast<int>(text.size()) || text[position++] != ':')
        {
            return false;
        }
        whitespace(text, position);
        if (position >= static_cast<int>(text.size()))
        {
            return false;
        }
        if (text[position] == '"')
        {
            field.kind = "STRING";
            if (!quoted(text, position, field.text))
            {
                return false;
            }
        }
        else if (text[position] == '[')
        {
            field.kind = "ARRAY";
            position++;
            whitespace(text, position);
            while (position < static_cast<int>(text.size()) && text[position] != ']')
            {
                double value = 0;
                if (!numeric(text, position, value) || value < INT_MIN || value > INT_MAX || std::floor(value) != value)
                {
                    return false;
                }
                field.array.append(static_cast<int>(value));
                if (field.array.getCount() > 10000)
                {
                    return false;
                }
                whitespace(text, position);
                if (position < static_cast<int>(text.size()) && text[position] == ',')
                {
                    position++;
                    whitespace(text, position);
                    if (position >= static_cast<int>(text.size()) || text[position] == ']')
                    {
                        return false;
                    }
                }
                else
                {
                    break;
                }
            }
            if (position >= static_cast<int>(text.size()) || text[position++] != ']')
            {
                return false;
            }
        }
        else
        {
            field.kind = "NUMBER";
            if (!numeric(text, position, field.number))
            {
                return false;
            }
        }
        count++;
        whitespace(text, position);
        if (position < static_cast<int>(text.size()) && text[position] == ',')
        {
            position++;
            whitespace(text, position);
            if (position >= static_cast<int>(text.size()) || text[position] == '}')
            {
                return false;
            }
        }
        else
        {
            break;
        }
    }
    if (position >= static_cast<int>(text.size()) || text[position++] != '}')
    {
        return false;
    }
    whitespace(text, position);
    return position == static_cast<int>(text.size());
}

bool JsonDocument::load(const std::string& path)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
    {
        return false;
    }
    std::string text;
    char letter = 0;
    while (input.get(letter))
    {
        text += letter;
        if (text.size() > 1000000)
        {
            return false;
        }
    }
    return parse(text);
}

const JsonDocument::Field* JsonDocument::find(const std::string& name) const
{
    for (int i = 0; i < count; i++)
    {
        if (fields[i].name == name)
        {
            return &fields[i];
        }
    }
    return NULL;
}

bool JsonDocument::string(const std::string& name, std::string& value) const
{
    const Field* field = find(name);
    if (field == NULL || field->kind != "STRING")
    {
        return false;
    }
    value = field->text;
    return true;
}

bool JsonDocument::number(const std::string& name, double& value) const
{
    const Field* field = find(name);
    if (field == NULL || field->kind != "NUMBER")
    {
        return false;
    }
    value = field->number;
    return true;
}

bool JsonDocument::integer(const std::string& name, int& value) const
{
    double result = 0;
    if (!number(name, result) || result < INT_MIN || result > INT_MAX || std::floor(result) != result)
    {
        return false;
    }
    value = static_cast<int>(result);
    return true;
}

bool JsonDocument::array(const std::string& name, IntBuffer& value) const
{
    const Field* field = find(name);
    if (field == NULL || field->kind != "ARRAY")
    {
        return false;
    }
    value = field->array;
    return true;
}

bool JsonDocument::only(const std::string& names) const
{
    for (int i = 0; i < count; i++)
    {
        if (names.find("|" + fields[i].name + "|") == std::string::npos)
        {
            return false;
        }
    }
    return true;
}
