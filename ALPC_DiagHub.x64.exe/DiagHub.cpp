#include "rpc_h.h"
#include <fstream>
#include <comdef.h>
#include <sddl.h>
#include <strsafe.h>
#include "ScopedHandle.h"

#pragma comment(lib, "rpcrt4.lib")

GUID CLSID_CollectorService =
{ 0x42CBFAA7, 0xA4A7, 0x47BB,{ 0xB4, 0x22, 0xBD, 0x10, 0xE9, 0xD0, 0x27, 0x00, } };

class __declspec(uuid("f23721ef-7205-4319-83a0-60078d3ca922")) ICollectionSession : public IUnknown {
public:

	virtual HRESULT __stdcall PostStringToListener(REFGUID, LPWSTR) = 0;
	virtual HRESULT __stdcall PostBytesToListener() = 0;
	virtual HRESULT __stdcall AddAgent(LPWSTR path, REFGUID) = 0;
	//.rdata:0000000180035868                 dq offset ? Start@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJPEAUtagVARIANT@@@Z; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::Start(tagVARIANT *)
	//.rdata:0000000180035870                 dq offset ? GetCurrentResult@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJFPEAUtagVARIANT@@@Z; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::GetCurrentResult(short, tagVARIANT *)
	//.rdata:0000000180035878                 dq offset ? Pause@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJXZ; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::Pause(void)
	//.rdata:0000000180035880                 dq offset ? Resume@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJXZ; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::Resume(void)
	//.rdata:0000000180035888                 dq offset ? Stop@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJPEAUtagVARIANT@@@Z; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::Stop(tagVARIANT *)
	//.rdata:0000000180035890                 dq offset ? TriggerEvent@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJW4SessionEvent@@PEAUtagVARIANT@@11@Z; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::TriggerEvent(SessionEvent, tagVARIANT *, tagVARIANT *, tagVARIANT *)
	//.rdata:0000000180035898                 dq offset ? GetGraphDataUpdates@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJAEBU_GUID@@PEAUtagSAFEARRAY@@PEAUGraphDataUpdates@@@Z; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::GetGraphDataUpdates(_GUID const &, tagSAFEARRAY *, GraphDataUpdates *)
	//.rdata:00000001800358A0                 dq offset ? QueryState@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJPEAW4SessionState@@@Z; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::QueryState(SessionState *)
	//.rdata:00000001800358A8                 dq offset ? GetStatusChangeEventName@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJPEAPEAG@Z; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::GetStatusChangeEventName(ushort * *)
	//.rdata:00000001800358B0                 dq offset ? GetLastError@EtwCollectionSession@StandardCollector@DiagnosticsHub@Microsoft@@UEAAJPEAJ@Z; Microsoft::DiagnosticsHub::StandardCollector::EtwCollectionSession::GetLastError(long *)
	//.rdata:00000001800358B8                 dq offset ? SetClientDelegate@EtwCollectionSession@StandardCollector@DiagnosticsHub@Mic
};

void ThrowOnError(HRESULT hr)
{
	if (hr != 0)
	{
		throw _com_error(hr);
	}
}

struct SessionConfiguration
{
	DWORD version; // Needs to be 1
	DWORD  a1;     // Unknown
	DWORD  something; // Also unknown
	DWORD  monitor_pid;
	GUID   guid;
	BSTR   path;    // Path to a valid directory
	CHAR   trailing[256];
};

class __declspec(uuid("7e912832-d5e1-4105-8ce1-9aadd30a3809")) IStandardCollectorClientDelegate : public IUnknown
{
};

class __declspec(uuid("0d8af6b7-efd5-4f6d-a834-314740ab8caa")) IStandardCollectorService : public IUnknown
{
public:
	virtual HRESULT __stdcall CreateSession(SessionConfiguration *, IStandardCollectorClientDelegate *, ICollectionSession **) = 0;
	virtual HRESULT __stdcall GetSession(REFGUID, ICollectionSession **) = 0;
	virtual HRESULT __stdcall DestroySession(REFGUID) = 0;
	virtual HRESULT __stdcall DestroySessionAsync(REFGUID) = 0;
	virtual HRESULT __stdcall AddLifetimeMonitorProcessIdForSession(REFGUID, int) = 0;
};

_COM_SMARTPTR_TYPEDEF(IStandardCollectorService, __uuidof(IStandardCollectorService));
_COM_SMARTPTR_TYPEDEF(ICollectionSession, __uuidof(ICollectionSession));


class CoInit
{
public:
	CoInit() {
		CoInitialize(nullptr);
	}

	~CoInit() {
		CoUninitialize();
	}
};

void LoadDll()
{
	CoInit coinit;
	try
	{
		GUID name;
		CoCreateGuid(&name);
		LPOLESTR name_str;
		StringFromIID(name, &name_str);

		WCHAR valid_dir[MAX_PATH];
		GetModuleFileName(nullptr, valid_dir, MAX_PATH);
		WCHAR* p = wcsrchr(valid_dir, L'\\');
		*p = 0;
		StringCchCat(valid_dir, MAX_PATH, L"\\etw");
		CreateDirectory(valid_dir, nullptr);

		IStandardCollectorServicePtr service;
		CoCreateInstance(CLSID_CollectorService, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&service));
		DWORD authn_svc;
		DWORD authz_svc;
		LPOLESTR principal_name;
		DWORD authn_level;
		DWORD imp_level;
		RPC_AUTH_IDENTITY_HANDLE identity;
		DWORD capabilities;

		CoQueryProxyBlanket(service, &authn_svc, &authz_svc, &principal_name, &authn_level, &imp_level, &identity, &capabilities);
		CoSetProxyBlanket(service, authn_svc, authz_svc, principal_name, authn_level, RPC_C_IMP_LEVEL_IMPERSONATE, identity, capabilities);
		SessionConfiguration config = {};
		config.version = 1;
		config.monitor_pid = ::GetCurrentProcessId();
		CoCreateGuid(&config.guid);
		bstr_t path = valid_dir;
		config.path = path;
		ICollectionSessionPtr session;

		service->CreateSession(&config, nullptr, &session);
		GUID agent_guid;
		CoCreateGuid(&agent_guid);
		session->AddAgent(L"license.rtf", agent_guid);
	}
	catch (const _com_error& error)
	{
		if (error.Error() == 0x8007045A)
		{
			printf("DLL should have been loaded\n");
		}
		else
		{
			printf("%ls\n", error.ErrorMessage());
			printf("%08X\n", error.Error());
		}
	}
}
