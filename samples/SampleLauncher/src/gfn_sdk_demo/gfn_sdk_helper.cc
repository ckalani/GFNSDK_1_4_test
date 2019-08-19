// Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.  Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "gfn_sdk_demo/gfn_sdk_helper.h"

#include "include/cef_parser.h"
#include "GfnRuntimeSdk_CAPI.h"

CefString GFN_SDK_INIT = "GFN_SDK_INIT";
CefString GFN_SDK_LAUNCH_GAME = "GFN_SDK_LAUNCH_GAME";
CefString GFN_SDK_IS_RUNNING_IN_CLOUD = "GFN_SDK_IS_RUNNING_IN_CLOUD";
CefString GFN_SDK_REQUEST_GFN_ACCESS_TOKEN = "GFN_SDK_REQUEST_GFN_ACCESS_TOKEN";
CefString GFN_SDK_GET_CLIENT_IP = "GFN_SDK_GET_CLIENT_IP";

static CefString DictToJson(CefRefPtr<CefDictionaryValue> dict)
{
    auto json = CefValue::Create();
    json->SetDictionary(dict);
    return CefWriteJSON(json, JSON_WRITER_DEFAULT);
}

static CefString GfnRuntimeErrorToString(GfnRuntimeSdk::GfnRuntimeError err)
{
    switch (err)
    {
    case GfnRuntimeSdk::GfnRuntimeError::gfnSuccess: return "Success";
    case GfnRuntimeSdk::GfnRuntimeError::gfnInitSuccessClientOnly: return "SDK initialization successful, but only client-side functionality available";
    case GfnRuntimeSdk::GfnRuntimeError::gfnInitFailure: return "SDK initialization failure";
    case GfnRuntimeSdk::GfnRuntimeError::gfnDllNotPresent: return "DLL Not Present";
    case GfnRuntimeSdk::GfnRuntimeError::gfnComError: return "Com Error";
    case GfnRuntimeSdk::GfnRuntimeError::gfnErrorCallingDLLFunction: return "Error Calling DLL Function";
    case GfnRuntimeSdk::GfnRuntimeError::gfnIncompatibleVersion: return "Incompatible Version";
    case GfnRuntimeSdk::GfnRuntimeError::gfnUnableToAllocateMemory: return "Unable To Allocate Memory";
    case GfnRuntimeSdk::GfnRuntimeError::gfnInvalidParameter: return "Invalid Parameter";
    case GfnRuntimeSdk::GfnRuntimeError::gfnInternalError: return "Internal Error";
    case GfnRuntimeSdk::GfnRuntimeError::gfnUnsupportedAPICall: return "Unsupported API Call";
    case GfnRuntimeSdk::GfnRuntimeError::gfnInvalidToken: return "Invalid Token";
    case GfnRuntimeSdk::GfnRuntimeError::gfnTimedOut: return "Timed Out";
    case GfnRuntimeSdk::GfnRuntimeError::gfnClientDownloadFailed: return "GFN Client download failed";
    default: return "Unknown Error";
    }
}

static GfnRuntimeSdk::GfnRuntimeError initGFN()
{
    static bool isInitialized = false;
    GfnRuntimeSdk::GfnRuntimeError err = GfnRuntimeSdk::GfnRuntimeError::gfnSuccess;

    if (isInitialized)
        return err;

    if ((err = GfnRuntimeSdk::gfnInitializeRuntimeSdk()) == GfnRuntimeSdk::GfnRuntimeError::gfnSuccess)
    {
        LOG(INFO) << "succeeded at initializing GFNLinkSDK";
        isInitialized = true;
        return err;
    }

    LOG(ERROR) << "error initializing: " << GfnRuntimeErrorToString(err);
    return err;
}

bool GfnSdkHelper(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    int64 query_id,
    const CefString& request,
    bool persistent,
    CefRefPtr<CefMessageRouterBrowserSide::Callback> callback)
{
    CefRefPtr<CefValue> requestValue = CefParseJSON(request, JSON_PARSER_RFC);

    CefRefPtr<CefDictionaryValue> dict = requestValue->GetDictionary();
    CefString command = dict->GetString("command");

    /**
     * Query command for initializing the GFN SDK. Should be called once during launcher startup
     * before making any other GFN SDK calls.
     */
    if (command == GFN_SDK_INIT)
    {
        GfnRuntimeSdk::GfnRuntimeError err = initGFN();
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetBool("success", err == GfnRuntimeSdk::GfnRuntimeError::gfnSuccess);
        response_dict->SetString("errorMessage", GfnRuntimeErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    /**
     * Calls into GFN SDK to determine whether the sample launcher is being executed inside
     * an NVIDIA GeForce NOW game seat.
     */
    else if (command == GFN_SDK_IS_RUNNING_IN_CLOUD)
    {
        bool enabled = GfnRuntimeSdk::gfnIsRunningInCloud();
        LOG(INFO) << "is enabled: " << enabled;

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetBool("enabled", enabled);

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    /**
     * Calls into GFN SDK to initiate a new GeForce NOW streaming session and launch the specified
     * game application. If a valid delegate token is passed in as a parameter it will automatically
     * authenticate the streaming session and begin streaming immediately. If an empty or invalid
     * delegate token is passed in then the GeForce NOW streaming client will display a login
     * window prompting the user to authenticate first.
     */
    else if (command == GFN_SDK_LAUNCH_GAME)
    {
        bool gameLaunched = false;
        std::string msg;
        if (dict->HasKey("serviceId") && dict->HasKey("appId") && dict->HasKey("authToken"))
        {
            GfnRuntimeSdk::StartStreamResponse response;
            GfnRuntimeSdk::StartStreamInput startStreamInput = { 0 };
            std::string pchServiceId = dict->GetString("serviceId").ToString();
            startStreamInput.pchServiceId = pchServiceId.c_str();
            std::string pchAppId = dict->GetString("appId").ToString();
            startStreamInput.pchAppId = pchAppId.c_str();
            std::string pchAuthToken = dict->GetString("authToken").ToString();
            startStreamInput.pchAuthToken = pchAuthToken.c_str();
            startStreamInput.pchCustomData = "This is example custom data";

            GfnRuntimeSdk::GfnRuntimeError err = GfnRuntimeSdk::gfnStartStream(&startStreamInput, &response);
            msg = "gfnStartStream = " + std::string(GfnRuntimeErrorToString(err));
            if (err != GfnRuntimeSdk::GfnRuntimeError::gfnSuccess)
            {
                LOG(ERROR) << "launch game error: " << msg;
            }
            else
            {
                gameLaunched = true;
                msg = msg + ", GFN Downloaded & Installed = " + (response.downloaded ? "Yes" : "Not needed");
                LOG(INFO) << "launch game response. Downloaded GeForceNOW? : " << response.downloaded;
            }
        }
        else
        {
            msg = "Bad arguments to CEF extension";
        }
    
        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetBool("gameLaunched", gameLaunched);
        response_dict->SetString("errorMessage", CefString(msg.c_str()));

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    /**
     * Calls into GFN SDK to request a GeForce NOW access token. This function is meant to be
     * used only when running inside the GeForce NOW game seat, and does not return a valid token
     * when run on the client side.
     * The delegate token returned by the call is issued specifically for the calling SDK client
     * and can only be redeemed by that client. Once redeemed the access token can then be used
     * to query information such as the NVIDIA Identity Services user ID to facilitate Account Link
     * lookup for Single Sign-On purposes.
     * Note that delegate tokens are single use only and can only be used or redeemed once, after
     * that another new delegate token needs to be retrieved.
     */
    else if (command == GFN_SDK_REQUEST_GFN_ACCESS_TOKEN)
    {
        const char* result;
        GfnRuntimeSdk::GfnRuntimeError err = GfnRuntimeSdk::gfnRequestGfnAccessToken(&result);
        if (err != GfnRuntimeSdk::GfnRuntimeError::gfnSuccess)
        {
            LOG(ERROR) << "token error: " << GfnRuntimeErrorToString(err);
        }
        else
        {
            LOG(INFO) << "token response: " << result;
        }

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetString("delegateToken", result);
        response_dict->SetString("errorMessage", GfnRuntimeErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);
        return true;
    }
    /**
     * Calls into GFN SDK to get the user's current local IP address. This is meant to be
     * called while running on the GeForce NOW game seat to handle scenarios where the IP
     * addressed is used for account security or other validation purposes or things like
     * region specific localization or other UI.
     */
    else if (command == GFN_SDK_GET_CLIENT_IP)
    {
        char const* clientIp;
        auto err = GfnRuntimeSdk::gfnGetClientIp(&clientIp);
        if (err != GfnRuntimeSdk::GfnRuntimeError::gfnSuccess)
        {
            LOG(ERROR) << "get client IP error: " << GfnRuntimeErrorToString(err);
        }
        else
        {
            LOG(INFO) << "client ip: " << clientIp;
        }

        CefRefPtr<CefDictionaryValue> response_dict = CefDictionaryValue::Create();
        response_dict->SetString("clientIp", clientIp);
        response_dict->SetString("errorMessage", GfnRuntimeErrorToString(err));

        CefString response(DictToJson(response_dict));
        callback->Success(response);

        return true;
    }

    return false;
}