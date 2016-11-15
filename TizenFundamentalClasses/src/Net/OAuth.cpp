/*
 * OAuth.cpp
 *
 *  Created on: Nov 14, 2016
 *      Author: Gilang M. Hamidy (g.hamidy@samsung.com)
 */

#include "TFC/Net/OAuth.h"

#define OAUTH_DECLARE_ERROR_MSG(ERR_CODE, ERR_STR) \
	template<> char const* TFC::Net::OAuth2ErrorSelector< ERR_CODE >::ErrorMessage = "OAuth2 Error: " ERR_STR

#define OAUTH_OP_CHECK_INIT int result
#define OAUTH_OP_CHECK_THROW(STATEMENT) result = STATEMENT; if (result != OAUTH2_ERROR_NONE) throw OAuth2Exception(result)

TFC ::Net::OAuth2Exception::OAuth2Exception(int oauthErrorCode) :
		TFCException(GetErrorMessage(oauthErrorCode)),
		errorCode(oauthErrorCode)
{
}

const char* TFC::Net::OAuth2Exception::GetErrorMessage(int code)
{
	switch(code)
	{
	case OAUTH2_ERROR_NONE: 				return "None";
	case OAUTH2_ERROR_OUT_OF_MEMORY: 		return "Out of memory";
	case OAUTH2_ERROR_INVALID_PARAMETER: 	return "Invalid parameter";
	case OAUTH2_ERROR_ALREADY_IN_PROGRESS: 	return "Already in progress";
	case OAUTH2_ERROR_NOT_SUPPORTED: 		return "Not supported";
	case OAUTH2_ERROR_PERMISSION_DENIED: 	return "Permission denied";
	case OAUTH2_ERROR_NETWORK_ERROR: 		return "Network error";
	case OAUTH2_ERROR_SERVER: 				return "Server error";
	case OAUTH2_ERROR_USER_CANCELLED: 		return "User cancelled";
	case OAUTH2_ERROR_VALUE_NOT_FOUND: 		return "Value not found";
	case OAUTH2_ERROR_UNKNOWN: 				return "Unknown error";
	}
	return "";
}

void TFC::Net::OAuth2ClientBase::PerformRequest()
{
	PerformXAuthRequest("", "");
}

void TFC::Net::OAuth2ClientBase::PerformXAuthRequest(
		const std::string& username, const std::string& password)
{
	if (paramPtr == nullptr) return;
	OAUTH_OP_CHECK_INIT;

	OAUTH_OP_CHECK_THROW(oauth2_request_set_auth_end_point_url(requestHandle, paramPtr->authUrl));
	if (paramPtr->tokenUrl)
	{
		OAUTH_OP_CHECK_THROW(oauth2_request_set_token_end_point_url(requestHandle, paramPtr->tokenUrl));
	}
	OAUTH_OP_CHECK_THROW(oauth2_request_set_redirection_url(requestHandle, paramPtr->redirectionUrl));

	OAUTH_OP_CHECK_THROW(oauth2_request_set_client_id(requestHandle, paramPtr->clientId));
	if (paramPtr->clientSecret)
	{
		OAUTH_OP_CHECK_THROW(oauth2_request_set_client_secret(requestHandle, paramPtr->clientSecret));
	}
	OAUTH_OP_CHECK_THROW(oauth2_request_set_scope(requestHandle, paramPtr->clientScope));
	OAUTH_OP_CHECK_THROW(oauth2_request_set_response_type(requestHandle, OAUTH2_RESPONSE_TYPE_CODE));

	if (username.length() > 0 && password.length() > 0)
	{
		OAUTH_OP_CHECK_THROW(oauth2_request_set_user_name(requestHandle, username.c_str()));
		OAUTH_OP_CHECK_THROW(oauth2_request_set_password(requestHandle, password.c_str()));
	}
	OAUTH_OP_CHECK_THROW(oauth2_manager_request_token(managerHandle, requestHandle, &RequestAuthorizationCallback, this));
}

void TFC::Net::OAuth2ClientBase::RefreshToken(std::string oldToken) {
}

TFC::Net::OAuth2ClientBase::OAuth2ClientBase(OAuthParam* param) :
		paramPtr(param),
		busy(false)
{
	OAUTH_OP_CHECK_INIT;
	OAUTH_OP_CHECK_THROW(oauth2_manager_create(&this->managerHandle));
	OAUTH_OP_CHECK_THROW(oauth2_request_create(&this->requestHandle));
}

TFC::Net::OAuth2ClientBase::~OAuth2ClientBase() {


	CleanUpRequest();

	// Clean up manager
	if(this->managerHandle != nullptr)
	{
		oauth2_manager_destroy(this->managerHandle);
		this->managerHandle = nullptr;
	}

	// Clean up param
	if(this->paramPtr != nullptr)
	{
		delete this->paramPtr;
		this->paramPtr = nullptr;
	}
}

void TFC::Net::OAuth2ClientBase::CleanUpRequest() {
	if(this->requestHandle != nullptr)
	{
		oauth2_request_destroy(this->requestHandle);
		this->requestHandle = nullptr;
	}
}

void TFC::Net::OAuth2ClientBase::RequestAuthorizationCallback(
		oauth2_response_h response, void* thisObj)
{
	char* token;
	OAUTH_OP_CHECK_INIT;
	OAUTH_OP_CHECK_THROW(oauth2_response_get_access_token(response, &token));

	auto oAuth2ClientPtr = static_cast<OAuth2ClientBase*>(thisObj);
	oAuth2ClientPtr->eventAccessTokenReceived(oAuth2ClientPtr, token);
}