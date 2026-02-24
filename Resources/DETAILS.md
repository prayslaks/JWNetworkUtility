Features:
* JWT Authentication with automatic 401 token refresh — queues pending requests per service type, re-sends all on success
* Blueprint-compatible HTTP API calls with cancellable request handles and wildcard USTRUCT parsing (ConvertJsonStringToStruct / ConvertStructToJsonString)
* Configurable per-request retry, timeout, and cancellation via FJWNU_RequestConfig and UJWNU_HttpRequestJobHandle
* Secure refresh token persistence using Windows DPAPI encryption with device-specific entropy
* Multi-server host management via INI config supporting independent GameServer and AuthServer endpoints

Code Modules:
* JWNetworkUtility (Runtime)
* JWNetworkUtilityTest (Runtime)

Number of Blueprints: 8

Number of C++ Classes: 14

Network Replicated: No

Supported Development Platforms:
* Windows: Yes
* Mac: No

Supported Target Build Platforms: Windows (Win64)

Documentation Link: https://github.com/prayslaks/JWNetworkUtility

Example Project: https://drive.google.com/file/d/1qaSRXXtwq70WWJbZEHpXQWTVB6HNDZ4A/view?usp=drive_link

Important/Additional Notes:
* Engine Version: Unreal Engine 5.6
* Refresh token encryption uses Windows DPAPI, which is Windows-only. Mac and Linux are not supported.
* The JWNetworkUtilityTest module (demo actors and sample Blueprints) is separated from the runtime module and not included in shipped builds.
* A FastAPI test server is bundled under TestServer/ for local development and demonstrating the full auth flow.
