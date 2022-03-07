#include "TestMpfrHelper.h"

#include "mpfr.h"
#include "gradido_blockchain/MemoryManager.h"
#include "gradido_blockchain/lib/Decay.h"

namespace testMpfrHelper {
	int compare(const std::string& strFloat, float value)
	{
		auto valueInput = MathMemory::create();
		mpfr_set_str(valueInput->getData(), strFloat.data(), 10, gDefaultRound);
		return mpfr_cmp_si(valueInput->getData(), value);
	}
}
