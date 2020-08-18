// memtool.cpp
//
// Simple tool for manipulating virtual memory.
//
// Usage
//  memtool.exe

#include <windows.h>

#include <map>
#include <string>
#include <variant>
#include <iostream>

#include <wdl/memory/util.hpp>
#include <wdl/memory/virtual.hpp>

constexpr const auto STATUS_SUCCESS_I = 0x0;
constexpr const auto STATUS_FAILURE_I = 0x1;

using region_map = std::map<unsigned long, wdl::memory::region>;

struct state_t
{
    unsigned long count{0};
    region_map    regions;
};

enum class command_type
{
    invalid,
    quit,
    reserve,
    commit,
    release,
    list
};

struct region_size
{
    SIZE_T size;
    region_size(SIZE_T size_) : size{size_} {}

    explicit operator SIZE_T() const
    {
        return size;
    }

    friend std::ostream& operator<<(std::ostream& os, region_size const& r)
    {
        os << r.size;
        return os;
    }
};

struct region_id
{
    unsigned long id;
    region_id(unsigned long id_) : id{id_} {}

    explicit operator unsigned long() const
    {
        return id;
    }

    friend std::ostream& operator<<(std::ostream& os, region_id const& r)
    {
        os << r.id;
        return os;
    } 
};

struct command
{
    command_type type;
    std::variant<
        std::monostate, 
        region_size, 
        region_id> payload;
};

command parse_command(std::string const& input)
{
    auto cmd = command{};

    auto pos = input.find(" ");
    if (pos == std::string::npos)
    {
        // no space delimiters in string
        if (input == "quit" || input == "exit")
        {
            cmd.type = command_type::quit;
        }
        else if (input == "list")
        {
            cmd.type = command_type::list;
        }
        else
        {
            cmd.type = command_type::invalid;
        }

        return cmd;
    }

    // space delimiter was found
    auto type_str    = input.substr(0, pos);
    auto payload_str = input.substr(pos, input.length() - type_str.length());

    try
    {
        if (type_str == "reserve")
        {
            cmd.type = command_type::reserve;
            auto size = std::stoull(payload_str);
            cmd.payload.emplace<region_size>(size);
        }
        else if (type_str == "commit")
        {
            cmd.type = command_type::commit;
            auto size = std::stoull(payload_str);
            cmd.payload.emplace<region_size>(size);
        }
        else if (type_str == "release")
        {
            cmd.type = command_type::release;
            auto id = std::stoul(payload_str);
            cmd.payload.emplace<region_id>(id);
        }
        else
        {
            cmd.type = command_type::invalid;
        }
    }
    catch (std::exception const&)
    {
        // sloppy way to handle:
        //  - std::invalid_argument
        //  - std::out_of_range
        // thrown by std::stoull
        cmd.type = command_type::invalid;
    }

    return cmd;
}

void handle_reserve(state_t& state, command const& cmd)
{
    std::cout << "Reserving " 
        << std::get<region_size>(cmd.payload) 
        << " pages" << std::endl;

    auto n_pages = static_cast<SIZE_T const>(std::get<region_size>(cmd.payload));
    auto size = n_pages * wdl::memory::page_size();
    
    auto r = wdl::memory::region::reserve(size, PAGE_READWRITE);
    if (!r)
    {
        // failed to commit new memory region
        std::cout << "Failed to reserve memory region\n"
            << "\tError: " << std::hex << ::GetLastError() << '\n';
        return;
    }

    // successfully reserved new region
    auto key = state.count++;
    state.regions.emplace(key, std::move(r));

    std::cout << "Reserved " << n_pages 
        << " page region with ID "
        << key << std::endl;
}

void handle_commit(state_t& state, command const& cmd)
{
    std::cout << "Committing " 
        << std::get<region_size>(cmd.payload) 
        << " pages" << std::endl;

    auto n_pages = static_cast<SIZE_T const>(std::get<region_size>(cmd.payload));
    auto size = n_pages * wdl::memory::page_size();
    
    auto r = wdl::memory::region::commit(size, PAGE_READWRITE);
    if (!r)
    {
        // failed to commit new memory region
        std::cout << "Failed to commit memory region\n"
            << "\tError: " << std::hex << ::GetLastError() << '\n';
        return;
    }

    // successfully committed new region
    auto key = state.count++;
    state.regions.emplace(key, std::move(r));
    
    std::cout << "Committed " << n_pages 
        << " page region with ID "
        << key << std::endl;
}

void handle_release(state_t& state, command const& cmd)
{
    std::cout << "Releasing region with ID " 
        << std::get<region_id>(cmd.payload) << std::endl;

    auto id = static_cast<unsigned long>(std::get<region_id>(cmd.payload));

    auto it = state.regions.find(id);
    if (it == std::end(state.regions))
    {
        std::cout << "Region with ID " 
            << std::get<region_id>(cmd.payload)
            << " not found" << std::endl;
        return;
    }

    auto n_pages = (*it).second.get_size() / wdl::memory::page_size();

    // region with specified base is in the map
    state.regions.erase(id);

    std::cout << "Region with ID " << std::get<region_id>(cmd.payload)
        << " (" << n_pages << " pages)"
        << " released" << std::endl;
}

void handle_list(state_t& state, command const& cmd)
{
    std::cout << "Listing all allocated regions:\n";
    for (auto const& e : state.regions)
    {
        auto type_str = 
            e.second.get_type() == wdl::memory::region_type::commit 
            ? "committed"
            : "reserved";

        std::cout << "[" << e.first << "] "
            << e.second.get_size() << " bytes " << type_str
            << " at address 0x" 
            << std::hex << e.second.get_base() << std::dec
            << '\n';
    }
}

void command_loop(state_t& state)
{
    auto run = true;

    while (run)
    {
        auto input = std::string{};

        std::cout << ">> ";
        std::getline(std::cin, input);

        auto cmd = parse_command(input);

        switch (cmd.type)
        {
        case command_type::invalid:
            std::cout << "Invalid command" << std::endl;
            break;
        case command_type::quit:
            run = false;
            break;
        case command_type::reserve:
            handle_reserve(state, cmd);
            break;
        case command_type::commit:
            handle_commit(state, cmd);
            break;
        case command_type::release:
            handle_release(state, cmd);
            break;
        case command_type::list:
            handle_list(state, cmd);
            break;
        default:
            break;
        }
    }
}

int main()
{
    auto state = state_t{};

    command_loop(state);

    std::cout << "Exiting" << std::endl;

    return STATUS_SUCCESS_I;
}