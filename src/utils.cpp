#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST

#include "utils.h"


/**
 * Trims the string from the left.
 */
void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}


/**
 * Trims the string from the right.
 */
void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}


/**
 * Trims from both ends (in place)
 * See: https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
 */
void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}


bool starts_with(const std::string &text, const std::string &prefix) {
    return text.find(prefix) == 0;
}


bool dir_exists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}


/**
 * Creates a list of directories as specified in the path.
 */
bool make_path(const std::string& path) {
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);

    if (ret == 0) {
        return true;
    }

    switch (errno) {
    case ENOENT:
        // parent didn't exist, try to create it
        {
            const auto pos = path.find_last_of('/');
            if (pos == std::string::npos) {
                return false;
            }
            if (!make_path( path.substr(0, pos) )) {
                return false;
            }
        }
        // now, try to create again
        return 0 == mkdir(path.c_str(), mode);

    case EEXIST:
        // done!
        return dir_exists(path);

    default:
        return false;
    }
    return true;
}
