#include "DecayDecimal.h"

#include "gradido_blockchain/MemoryManager.h"
#include <mpfr.h>

namespace plugin {
	namespace apollo {
		
		void DecayDecimal::applyDecay(Poco::Timespan duration)
		{

			Decimal durationSeconds(duration.totalSeconds());
			Decimal secondsPerYear(MAGIC_NUMBER_GREGORIAN_CALENDER_SECONDS_PER_YEAR);		
			Decimal timeFactor = (durationSeconds / secondsPerYear).toString(MAGIC_NUMBER_APOLLO_PRECISION);
			timeFactor = (2 ^ timeFactor).toString(MAGIC_NUMBER_APOLLO_PRECISION);
			(*this) /= timeFactor;
			mpfr_set_str(mDecimal, toString(MAGIC_NUMBER_APOLLO_PRECISION).data(), 10, gDefaultRound);

			//auto decay = calculateDecayFactor(duration);
			//(*this) *= decay;
		}

		bool DecayDecimal::isSimilarEnough(const Decimal& b)
		{
			Decimal diff = (*this) - b;
			mpfr_abs(diff, diff, gDefaultRound);
			if (mpfr_cmp_d(diff, MAGIC_NUMBER_DIFF_TO_NODE_DECAY_CALCULATION_THRESHOLD) > 0) {
				return false;
			}
			return true;
		}
	}
}