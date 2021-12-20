#include "NotificationList.h"

#include "../ServerConfig.h"

//#include "Poco/Net/MailMessage.h"
#include "Poco/Net/MediaType.h"
#include "Poco/DateTimeFormatter.h"



NotificationList::NotificationList()
	: mLogging(Poco::Logger::get("errorLog"))
{

}

NotificationList::~NotificationList()
{
	while (mErrorStack.size() > 0) {
		delete mErrorStack.top();
		mErrorStack.pop();
	}

}

void NotificationList::addError(Notification* error, bool log/* = true */)
{
#ifndef _TEST_BUILD
	if (log) {
		std::string dateTimeString = Poco::DateTimeFormatter::format(Poco::DateTime(), "%d.%m.%y %H:%M:%S");
		mLogging.error("%s [NotificationList::addError] %s", dateTimeString, error->getString(false));
	}
#endif
	mErrorStack.push(error);
}


void NotificationList::addNotification(Notification* notification)
{
	mErrorStack.push(notification);
}

Notification* NotificationList::getLastError()
{
	if (mErrorStack.size() == 0) {
		return nullptr;
	}

	Notification* error = mErrorStack.top();
	if (error) {
		mErrorStack.pop();
	}

	return error;
}


void NotificationList::clearErrors()
{
	while (mErrorStack.size()) {
		auto error = mErrorStack.top();
		if (error) {
			delete error;
		}
		mErrorStack.pop();
	}
}


int NotificationList::getErrors(NotificationList* send)
{
	Notification* error = nullptr;
	int iCount = 0;
	while (error = send->getLastError()) {
		addError(error, false);
		iCount++;
	}
	return iCount;
}


void NotificationList::printErrors()
{
	while (mErrorStack.size() > 0) {
		auto error = mErrorStack.top();
		mErrorStack.pop();
		printf(error->getString().data());
		delete error;
	}
}

std::vector<std::string> NotificationList::getErrorsArray()
{
	std::vector<std::string> result;
	result.reserve(mErrorStack.size());

	while (mErrorStack.size() > 0) {
		auto error = mErrorStack.top();
		mErrorStack.pop();
		//result->add(error->getString());
		result.push_back(error->getString());
		delete error;
	}
	return result;
}

rapidjson::Value NotificationList::getErrorsArray(rapidjson::Document::AllocatorType& alloc)
{
	rapidjson::Value value;
	value.SetArray();
	while (mErrorStack.size() > 0) {
		auto error = mErrorStack.top();
		mErrorStack.pop();
		value.PushBack(rapidjson::Value(error->getString().data(), alloc), alloc);
		delete error;
	}
	return value;
}


std::string NotificationList::getErrorsHtml()
{
	std::string res;
	res = "<ul class='grd-no-style'>";
	while (mErrorStack.size() > 0) {
		auto error = mErrorStack.top();
		mErrorStack.pop();
		if (error->isError()) {
			res += "<li class='grd-error'>";
		}
		else if (error->isSuccess()) {
			res += "<li class='grd-success'>";
		}
		res += error->getHtmlString();
		res += "</li>";
		delete error;
	}
	res += "</ul>";
	return res;
}

std::string NotificationList::getErrorsHtmlNewFormat()
{
	std::string html;
	
	while (mErrorStack.size() > 0) {
		auto error = std::unique_ptr<Notification>(mErrorStack.top());
		mErrorStack.pop();
		if (error->isError()) {
			html += "<div class=\"alert alert-error\" role=\"alert\">";
			html += "<i class=\"material-icons-outlined\">report_problem</i>";
		}
		else if (error->isSuccess()) {
			html += "<div class=\"alert alert-success\" role=\"alert\">";
		}
		html += "<span>";
		html += error->getHtmlString();
		html += "</span>";
		html += "</div>";
	}
	return html;
}
/*
<div class="alert alert-error" role="alert">
<i class="material-icons-outlined">report_problem</i>
<span>Der Empf?nger wurde nicht auf dem Login-Server gefunden, hat er sein Konto schon angelegt?</span>
</div>
*/

