#include "JsonCreationTransaction.h"

using namespace rapidjson;


	/*
	{
		"transactionType": "creation",
		"created":"2021-01-10 10:00:00",
		"memo": "AGE September 2021",
		"recipientName":"<apollo user identificator>",
		"amount": "100",
		"coinColor": "ffffffff",
		"targetDate": "2021-09-01 01:00:00",
	}
	and jwtoken with user informations
	*/

Document JsonCreationTransaction::handle(const rapidjson::Document& params)
{

}