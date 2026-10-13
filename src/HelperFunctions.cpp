#include "HelperFunctions.hpp"

namespace helper {

    int32_t int32tStringFind(const std::string& source, const char target) {
        int32_t pos = 0;
        for (std::string::const_iterator it = source.begin(); it != source.end(); it++) {
            if (*it == target) {
                return pos;
            }
            pos++;
        }

        //character not found in string
        pos = -1;
        return pos;
    }

    //string::size() also returns a size_t, so we cannot use O(1) size()
    //instead we use a O(n) c-style string method
    int32_t int32tStringSize(const std::string& source) {
        int32_t size = 0;
        for (std::string::const_iterator it = source.begin(); it != source.end(); it++) {
            size++;
        }
        return size;
    }

    //string_view overloaded function of same function above
    int32_t int32tStringSize(const std::string_view& source) {
        int32_t size = 0;
        for (std::string_view::const_iterator it = source.begin(); it != source.end(); it++) {
            size++;
        }
        return size;
    }

    //generic fatal error function
    //no return silences a lot of errors!
    [[noreturn]] void fatalError(const std::string& errMessage)
    {
        std::cerr << "Fatal Error: " << errMessage << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
