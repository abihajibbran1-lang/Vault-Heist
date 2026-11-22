#ifndef VAULT_SAVE_FILE_H
#define VAULT_SAVE_FILE_H

#include <string>

class SaveFile
{
private:
    static bool read(const std::string& path, std::string& content);

public:
    static bool commit(const std::string& temporary, const std::string& destination);
};

#endif
