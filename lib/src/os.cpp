#include "krico/backup/os.h"
#include <unistd.h>
#include <pwd.h>

using namespace krico::backup;

std::string krico::backup::get_username() {
    passwd *pwd;
    uid_t userid = getuid();
    pwd = getpwuid(userid);
    return pwd->pw_name;
}
