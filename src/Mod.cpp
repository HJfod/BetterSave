#include "Mod.hpp"
#include <Geode/utils/general.hpp>

using namespace geode::prelude;

static std::string convertToKebabCase(std::string const& str) {
	std::string res {};
	char last = '\0';
	for (auto c : str) {
		// Add a dash if the character is in uppercase (camelCase / PascalCase) 
		// or a space (Normal case) or an underscore (snake_case) and the 
		// built result string isn't empty and make sure there's only a 
		// singular dash
		// Don't add a dash if the previous character was also uppercase or a 
		// number (SCREAM1NG L33TCASE should be just scream1ng-l33tcase)
		if ((std::isupper(c) && !(std::isupper(last) || std::isdigit(last))) || std::isspace(c) || c == '_') {
			if (res.size() && res.back() != '-') {
				res.push_back('-');
			}
		}
		// Only preserve alphanumeric characters
		if (std::isalnum(c)) {
			res.push_back(std::tolower(c));
		}
		last = c;
	}
	// If there is a dash at the end (for example because the name ended in a 
	// space) then get rid of that
	if (res.back() == '-') {
		res.pop_back();
	}
	return res;
}
static void checkReservedFilenames(std::string& name) {
    switch (hash(name.c_str())) {
        case hash("con"): case hash("prn"): case hash("aux"): case hash("nul"):
        // This was in https://www.boost.org/doc/libs/1_36_0/libs/filesystem/doc/portability_guide.htm?
        // Never heard of it before though
        case hash("clock$"):
        case hash("com1"): case hash("com2"): case hash("com3"): case hash("com4"):
        case hash("com5"): case hash("com6"): case hash("com7"): case hash("com8"): case hash("com9"):
        case hash("lpt1"): case hash("lpt2"): case hash("lpt3"): case hash("lpt4"):
        case hash("lpt5"): case hash("lpt6"): case hash("lpt7"): case hash("lpt8"): case hash("lpt9"):
        {
            name += "-0";
        }
        break;

        default: {} break;
    }
}

std::string getFreeIDInDir(std::string const& orig, std::filesystem::path const& dir, std::string const& ext) {
    // Synthesize an ID for the level by taking the level name in kebab-case 
    // and then adding an incrementing number at the end until there exists 
    // no folder with the same name already
    auto name = convertToKebabCase(orig);
    
    // Prevent names that are too long (some people might use input bypass 
    // to give levels absurdly long names)
    if (name.size() > 20) {
        name = name.substr(0, 20);
    }
    if (name.empty()) {
        name = "unnamed";
    }

    // Check that no one has made a level called CON
    checkReservedFilenames(name);

    auto id = name + "." + ext;
    size_t counter = 0;

    while (std::filesystem::exists(dir / id)) {
        id = fmt::format("{}-{}", name, counter);
        counter += 1;
    }

    return id;
}
