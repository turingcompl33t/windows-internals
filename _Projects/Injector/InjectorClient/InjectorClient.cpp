// InjectorClient.cpp
// Simple commandline client for injector library.
//
// Usage:
//  InjectorClient.exe <PAYLOAD PATH> <TARGET PID> <METHOD INDEX>

#include <string>
#include <iostream>
#include <stdexcept>

#include <InjectorLib.h>

#pragma comment(lib, "InjectorLib.lib")

constexpr auto STATUS_SUCCESS_I = 0x0;
constexpr auto STATUS_FAILURE_I = 0x1;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cout << "[CLIENT]: Invalid arguments\n";
        std::cout << "[CLIENT]: Usage: " << argv[0];
        std::cout << " <PAYLOAD PATH> <TARGET PID> <METHOD INDEX>\n";
        return STATUS_FAILURE_I;
    }

    DWORD           dwTargetPid;
    DWORD           dwMethodIndex;
    InjectionStatus status;
    InjectionMethod method;

    std::unique_ptr<std::vector<std::string>> SupportedMethods;

    try
    {
        dwTargetPid   = std::stoul(argv[2]);
        dwMethodIndex = std::stoul(argv[3]);
    }
    catch (std::invalid_argument&)
    {
        std::cout << "[CLIENT]: Failed to parse arguments (invalid_argument)\n";
        return STATUS_FAILURE_I;
    }
    catch (std::out_of_range&)
    {
        std::cout << "[CLIENT]: Failed to parse arguments (out_of_range)\n";
        return STATUS_FAILURE_I;
    }

    std::cout << "[CLIENT]: Parsed injection request parameters:\n";
    std::cout << "\tPayload Path: " << argv[1] << '\n';
    std::cout << "\tTarget PID:   " << dwTargetPid << '\n';

    // initialize a new injector
    Injector injector{};

    method = injector.IndexToInjectionMethod(dwMethodIndex);
    if (InjectionMethod::InvalidMethod == method)
    {
        std::cout << "[CLIENT]: Invalid method index specified\n";
        return STATUS_FAILURE_I;
    }

    // perform the injection
    status = injector.Inject(
        argv[1],
        dwTargetPid,
        method
    );

    if (status != InjectionStatus::StatusSuccess)
    {
        std::cout << "[CLIENT]: Injection failed!\n";
    }
    else
    {
        std::cout << "[CLIENT]: Injection succeeded!\n";
    }

    return STATUS_SUCCESS_I;
}
