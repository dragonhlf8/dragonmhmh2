#include "AddressUtil.h"
#include <map>

/*static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const int8_t mapBase58[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
};*/

static const std::string BASE58_STRING =
		"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

struct Base58Map {
	static std::map<char, int> createBase58Map() {
		std::map<char, int> m;
		for (int i = 0; i < 58; i++) {
			m[BASE58_STRING[i]] = i;
		}

		return m;
	}

	static std::map<char, int> myMap;
};

std::map<char, int> Base58Map::myMap = Base58Map::createBase58Map();

/**
 * Converts a base58 string to uint256
 */
secp256k1::uint256 Base58::toBigInt(const std::string &s) {
	secp256k1::uint256 value;

	for (size_t i = 0; i < s.length(); i++) {
		value = value.mul(58);

		int c = Base58Map::myMap[s[i]];
		value = value.add(c);
	}

	return value;
}

void Base58::toHash160(const std::string &s, unsigned int hash[5]) {
	secp256k1::uint256 value = toBigInt(s);
	unsigned int words[6];

	value.exportWords(words, 6, secp256k1::uint256::BigEndian);

	// Extract words, ignore checksum
	for (int i = 0; i < 5; i++) {
		hash[i] = words[i];
	}
}

bool Base58::isBase58(std::string s) {
	for (int i = 0; i < static_cast<int>(s.length()); i++) {
		if (static_cast<int>(BASE58_STRING.find(s[i])) < 0) {
			return false;
		}
	}

	return true;
}

std::string Base58::toBase58(const secp256k1::uint256 &x) {
	std::string s;

	secp256k1::uint256 value = x;

	for (unsigned int i=0; i<=32; i++) {
		secp256k1::uint256 digit = value.mod(58);
		int digitInt = digit.toInt32();

		s = BASE58_STRING[digitInt] + s;

		value = value.div(58);
	}

	return s;
}

void Base58::getMinMaxFromPrefix(const std::string &prefix,
		secp256k1::uint256 &minValueOut, secp256k1::uint256 &maxValueOut) {
	secp256k1::uint256 minValue = toBigInt(prefix);
	secp256k1::uint256 maxValue = minValue;
	int exponent = 1;

	// 2^192
	unsigned int expWords[] = { 0, 0, 0, 0, 0, 0, 1, 0 };

	secp256k1::uint256 exp(expWords);

	// Find the smallest 192-bit number that starts with the prefix. That is, the prefix multiplied
	// by some power of 58
	secp256k1::uint256 nextValue = minValue.mul(58);

	while (nextValue.cmp(exp) < 0) {
		exponent++;
		minValue = nextValue;
		nextValue = nextValue.mul(58);
	}

	secp256k1::uint256 diff = secp256k1::uint256(58).pow(exponent - 1).sub(1);

	maxValue = minValue.add(diff);

	if (maxValue.cmp(exp) > 0) {
		maxValue = exp.sub(1);
	}

	minValueOut = minValue;
	maxValueOut = maxValue;
}