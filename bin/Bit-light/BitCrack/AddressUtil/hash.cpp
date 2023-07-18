#include "AddressUtil.h"
#include "CryptoUtil.h"

bool Address::verifyAddress(std::string address) {
	// Check length
	if (address.length() > 34) {
		return false;
	}

	// Check encoding
	if (!Base58::isBase58(address)) {
		return false;
	}

	std::string noPrefix = address.substr(1);

	secp256k1::uint256 value = Base58::toBigInt(noPrefix);
	unsigned int words[6];
	unsigned int hash[5];
	unsigned int checksum;

	value.exportWords(words, 6, secp256k1::uint256::BigEndian);
	memcpy(hash, words, sizeof(unsigned int) * 5);
	checksum = words[5];

	return crypto::checksum(hash) == checksum;
}

std::string Address::fromPublicKey(const secp256k1::ecpoint &p,
		bool compressed) {
	unsigned int xWords[8] = { 0 };
	unsigned int yWords[8] = { 0 };

	p.x.exportWords(xWords, 8, secp256k1::uint256::BigEndian);
	p.y.exportWords(yWords, 8, secp256k1::uint256::BigEndian);

	unsigned int digest[5];

	if (compressed) {
		Hash::hashPublicKeyCompressed(xWords, yWords, digest);
	} else {
		Hash::hashPublicKey(xWords, yWords, digest);
	}

	unsigned int checksum = crypto::checksum(digest);

	unsigned int addressWords[8] = { 0 };
	for (int i = 0; i < 5; i++) {
		addressWords[2 + i] = digest[i];
	}
	addressWords[7] = checksum;

	secp256k1::uint256 addressBigInt(addressWords,
			secp256k1::uint256::BigEndian);

	return "1" + Base58::toBase58(addressBigInt);
}

void Hash::hashPublicKey(const secp256k1::ecpoint &p, unsigned int *digest) {
	unsigned int xWords[8];
	unsigned int yWords[8];

	p.x.exportWords(xWords, 8, secp256k1::uint256::BigEndian);
	p.y.exportWords(yWords, 8, secp256k1::uint256::BigEndian);

	hashPublicKey(xWords, yWords, digest);
}

void Hash::hashPublicKeyCompressed(const secp256k1::ecpoint &p,
		unsigned int *digest) {
	unsigned int xWords[8];
	unsigned int yWords[8];

	p.x.exportWords(xWords, 8, secp256k1::uint256::BigEndian);
	p.y.exportWords(yWords, 8, secp256k1::uint256::BigEndian);

	hashPublicKeyCompressed(xWords, yWords, digest);
}

void Hash::hashPublicKey(const unsigned int *x, const unsigned int *y,
		unsigned int *digest) {
	unsigned int msg[16];
	unsigned int sha256Digest[8];

	// 0x04 || x || y
	msg[15] = (y[7] >> 8u) | (y[6] << 24u);
	msg[14] = (y[6] >> 8u) | (y[5] << 24u);
	msg[13] = (y[5] >> 8u) | (y[4] << 24u);
	msg[12] = (y[4] >> 8u) | (y[3] << 24u);
	msg[11] = (y[3] >> 8u) | (y[2] << 24u);
	msg[10] = (y[2] >> 8u) | (y[1] << 24u);
	msg[9] = (y[1] >> 8u) | (y[0] << 24u);
	msg[8] = (y[0] >> 8u) | (x[7] << 24u);
	msg[7] = (x[7] >> 8u) | (x[6] << 24u);
	msg[6] = (x[6] >> 8u) | (x[5] << 24u);
	msg[5] = (x[5] >> 8u) | (x[4] << 24u);
	msg[4] = (x[4] >> 8u) | (x[3] << 24u);
	msg[3] = (x[3] >> 8u) | (x[2] << 24u);
	msg[2] = (x[2] >> 8u) | (x[1] << 24u);
	msg[1] = (x[1] >> 8u) | (x[0] << 24u);
	msg[0] = (x[0] >> 8u) | 0x04000000;

	crypto::sha256Init(sha256Digest);
	crypto::sha256(msg, sha256Digest);

	// Zero out the message
	for (int i = 0; i < 16; i++) {
		msg[i] = 0;
	}

	// Set first byte, padding, and length
	msg[0] = (y[7] << 24u) | 0x00800000;
	msg[15] = 65 * 8;

	crypto::sha256(msg, sha256Digest);

	for (int i = 0; i < 16; i++) {
		msg[i] = 0;
	}

	// Swap to little endian
	for (int i = 0; i < 8; i++) {
		msg[i] = be32toh(sha256Digest[i]);
	}

	// Message length, little endian
	msg[8] = 0x00000080;
	msg[14] = 256;
	msg[15] = 0;

	crypto::ripemd160(msg, digest);
}

void Hash::hashPublicKeyCompressed(const unsigned int *x, const unsigned int *y,
		unsigned int *digest) {
	unsigned int msg[16] = { 0 };
	unsigned int sha256Digest[8];

	// Compressed public key format
	msg[15] = 33 * 8;

	msg[8] = (x[7] << 24u) | 0x00800000;
	msg[7] = (x[7] >> 8u) | (x[6] << 24u);
	msg[6] = (x[6] >> 8u) | (x[5] << 24u);
	msg[5] = (x[5] >> 8u) | (x[4] << 24u);
	msg[4] = (x[4] >> 8u) | (x[3] << 24u);
	msg[3] = (x[3] >> 8u) | (x[2] << 24u);
	msg[2] = (x[2] >> 8u) | (x[1] << 24u);
	msg[1] = (x[1] >> 8u) | (x[0] << 24u);

	if (y[7] & 0x01) {
		msg[0] = (x[0] >> 8u) | 0x03000000;
	} else {
		msg[0] = (x[0] >> 8u) | 0x02000000;
	}

	crypto::sha256Init(sha256Digest);
	crypto::sha256(msg, sha256Digest);

	for (int i = 0; i < 16; i++) {
		msg[i] = 0;
	}

	// Swap to little endian
	for (int i = 0; i < 8; i++) {
		msg[i] = be32toh(sha256Digest[i]);
	}

	// Message length, little endian
	msg[8] = 0x00000080;
	msg[14] = 256;
	msg[15] = 0;

	crypto::ripemd160(msg, digest);
}
