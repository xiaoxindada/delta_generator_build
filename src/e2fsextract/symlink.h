#ifndef __EXTRACT_SYMLINK_H
#define __EXTRACT_SYMLINK_H

#include <string>

const char cygwin_symlink_magic[] = {
        '!', '<', 's', 'y', 'm', 'l', 'i', 'n', 'k', '>',
};
int xsymlink(const std::string link_target, const std::string path);
#endif //__EXTRACT_SYMLINK_H
