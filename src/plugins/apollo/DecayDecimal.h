#ifndef __GRADIDO_BLOCKCHAIN_CONNECTOR_PLUGINS_APOLLO_H
#define __GRADIDO_BLOCKCHAIN_CONNECTOR_PLUGINS_APOLLO_H

#include "gradido_blockchain/lib/DecayDecimal.h"

#define MAGIC_NUMBER_DIFF_TO_NODE_DECAY_CALCULATION_THRESHOLD 0.0005
#define MAGIC_NUMBER_APOLLO_DECAY_FACTOR "0.9999999780350404897320120"
#define MAGIC_NUMBER_GREGORIAN_CALENDER_SECONDS_PER_YEAR 31556952
#define MAGIC_NUMBER_APOLLO_PRECISION 25

namespace plugin {
	namespace apollo {
		class DecayDecimal : public ::DecayDecimal
		{
		public: 
			using ::DecayDecimal::DecayDecimal;
			static Decimal calculateDecayFactor(Poco::Timespan duration);
			void applyDecay(Poco::Timespan duration);

			bool isSimilarEnough(const Decimal& b);
		};
	}
}


#endif //__GRADIDO_BLOCKCHAIN_CONNECTOR_PLUGINS_APOLLO_H