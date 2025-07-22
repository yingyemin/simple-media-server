#include "T0A00_8A00.h"
#include <sstream>

// Getter 方法实现
int T0A00_8A00::getE() const {
    return e;
}

const std::vector<uint8_t>& T0A00_8A00::getN() const {
    return n;
}

// Setter 方法实现
void T0A00_8A00::setE(int eVal) {
    e = eVal;
}

void T0A00_8A00::setN(const std::vector<uint8_t>& nVal) {
    n = nVal;
    if (n.size() > 128) {
        n.resize(128);
    }
}

// toString 方法实现
std::string T0A00_8A00::toString() const {
    std::ostringstream oss;
    oss << "T0A00_8A00[";
    oss << "e=" << e << ", ";
    oss << "n=[";
    for (size_t i = 0; i < n.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << static_cast<int>(n[i]);
    }
    oss << "]]";
    return oss.str();
}