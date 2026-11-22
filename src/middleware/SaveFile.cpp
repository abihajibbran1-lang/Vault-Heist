#include "middleware/SaveFile.h"
#include <fstream>
#include <cstdio>

bool SaveFile::read(const std::string& path, std::string& content)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
    {
        return false;
    }
    content.clear();
    char letter = 0;
    while (input.get(letter))
    {
        content += letter;
        if (content.size() > 1000000)
        {
            return false;
        }
    }
    return !input.bad();
}

bool SaveFile::commit(const std::string& temporary, const std::string& destination)
{
    if (std::rename(temporary.c_str(), destination.c_str()) == 0)
    {
        return true;
    }
    // Windows/OneDrive readers can deny replacement while allowing writes.
    // Preserve the previous bytes before using the recoverable copy fallback.
    std::string content;
    if (!read(temporary, content))
    {
        return false;
    }
    std::ifstream existing(destination.c_str(), std::ios::binary);
    bool present = static_cast<bool>(existing);
    existing.close();
    if (present)
    {
        std::string previous;
        if (!read(destination, previous))
        {
            return false;
        }
        std::ofstream backup((destination + ".bak").c_str(), std::ios::binary);
        backup.write(previous.c_str(), static_cast<std::streamsize>(previous.size()));
        backup.close();
        if (backup.fail())
        {
            return false;
        }
    }
    std::ofstream output(destination.c_str(), std::ios::binary);
    output.write(content.c_str(), static_cast<std::streamsize>(content.size()));
    output.close();
    std::string verified;
    return !output.fail() && read(destination, verified) && verified == content;
}
