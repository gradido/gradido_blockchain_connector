#ifndef _GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_TEST_MPFR_HELPER_H
#define _GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_TEST_MPFR_HELPER_H

#include <string>

namespace testMpfrHelper
{
	int compare(const std::string& strFloat, float value);
	inline bool isSame(const std::string& strFloat, float value) { return compare(strFloat, value) == 0; }
}

#endif // _GRADIDO_BLOCKCHAIN_CONNECTOR_TEST_TEST_MPFR_HELPER_H