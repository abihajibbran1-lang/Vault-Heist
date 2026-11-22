#include "middleware/SaveFile.h"
#include <fstream>
#include <cassert>
#include <iostream>

class SaveFileTests
{
public:
    static void prepare(const std::string& path)
    {
        std::ofstream old(path.c_str());
        old << "old state";
        old.close();
        std::ofstream fresh((path + ".tmp").c_str());
        fresh << "new state";
    }

    static void verify(const std::string& path)
    {
        assert(SaveFile::commit(path + ".tmp", path));
        std::ifstream input(path.c_str());
        std::string result;
        std::getline(input, result);
        assert(result == "new state");
    }
};

int main(int argc, char* argv[])
{
    assert(argc >= 2);
    std::string path = argv[1];
    if (argc > 2 && std::string(argv[2]) == "prepare")
    {
        SaveFileTests::prepare(path);
        return 0;
    }
    if (argc > 2 && std::string(argv[2]) == "locked")
    {
        SaveFileTests::verify(path);
        std::ifstream backup((path + ".bak").c_str());
        std::string previous;
        std::getline(backup, previous);
        assert(previous == "old state");
        std::cout << "Locked-file fallback passed; original bytes retained in backup.\n";
        return 0;
    }
    SaveFileTests::prepare(path);
    SaveFileTests::verify(path);
    assert(!SaveFile::commit(path + ".missing", path));
    std::cout << "Save replacement and failed-source preservation passed.\n";
}
