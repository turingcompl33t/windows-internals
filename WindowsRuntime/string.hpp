// string.hpp
//
// Wrappers for Windows runtime strings.

#pragma once

#include <windows.h>
#include <hstring.h>
#include <winstring.h>

#include <wdl/debug.hpp>
#include <wdl/error/com_exception.hpp>
#include <wdl/handle/unique_handle.hpp>

struct wrt_string_handle_traits
{
    using pointer = HSTRING;

    constexpr static pointer invalid() noexcept
    {
        return nullptr;
    }

    static void close(pointer value) noexcept
    {
        VERIFY_(S_OK, ::WindowsDeleteString(value));
    }
};

using wrt_string = wdl::handle::unique_handle<wrt_string_handle_traits>;

struct wrt_string_buffer_handle_traits
{
    using pointer = HSTRING_BUFFER;

    constexpr static pointer invalid() noexcept
    {
        return nullptr;
    }

    static void close(pointer value) noexcept
    {
        VERIFY_(S_OK, ::WindowsDeleteStringBuffer(value));
    }
};

using wrt_string_buffer 
    = wdl::handle::unique_handle<wrt_string_buffer_handle_traits>;

class wrt_string_reference
{
    HSTRING        m_string;
    HSTRING_HEADER m_header;

public:
    wrt_string_reference(wchar_t const* const data, unsigned length)
    {
        auto result = ::WindowsCreateStringReference(
            data,
            length,
            &m_header,
            &m_string);

        wdl::error::check_com(result);
    }

    template <unsigned Count>
    wrt_string_reference(wchar_t const (&data)[Count])
    {
        auto result = ::WindowsCreateStringReference(
            data,
            Count - 1,
            &m_header,
            &m_string);

        wdl::error::check_com(result);
    }

    HSTRING get() const noexcept
    {
        return m_string;
    }

    wrt_string_reference(wrt_string_reference const&)            = delete;
    wrt_string_reference& operator=(wrt_string_reference const&) = delete;

    void* operator new(std::size_t)   = delete;
    void* operator new[](std::size_t) = delete;
    void operator delete(void*)   = delete;
    void operator delete[](void*) = delete;
};

wrt_string create_string(
    wchar_t const*      string,
    unsigned long const length
    )
{
    auto tmp = wrt_string{};

    auto result = ::WindowsCreateString(
        string,
        length,
        tmp.get_address_of());

    wdl::error::check_com(result);

    return tmp;
}

template <int Count>
wrt_string create_string(wchar_t const (&string)[Count])
{
    return create_string(string, Count - 1);
}

wchar_t const* buffer(wrt_string const& string)
{
    return ::WindowsGetStringRawBuffer(string.get(), nullptr);
}

wchar_t const* buffer(
    wrt_string const& string,
    unsigned&         length
    )
{
    return ::WindowsGetStringRawBuffer(string.get(), &length);
}

unsigned length(wrt_string const& string)
{
    return ::WindowsGetStringLen(string.get());
}

bool empty(wrt_string const& string)
{
    return ::WindowsIsStringEmpty(string.get());
}

wrt_string duplicate(wrt_string const& string)
{
    auto tmp = wrt_string{};

    auto result = ::WindowsDuplicateString(
        string.get(),
        tmp.get_address_of());

    wdl::error::check_com(result);

    return tmp;
}

wrt_string substring(
    wrt_string const& string,
    unsigned const    start
    )
{
    auto tmp = wrt_string{};

    auto result = ::WindowsSubstring(
        string.get(),
        start,
        tmp.get_address_of());

    wdl::error::check_com(result);

    return tmp;
}

template <typename... Args>
wrt_string format(wchar_t const* format, Args... args)
{
    // query the length of the resulting formatted string
    int const size = swprintf(nullptr, 0, format, args);

    if (-1 == size) 
        throw wdl::error::com_exception{E_INVALIDARG};

    if (0 == size)
        return wrt_string{};

   auto buffer     = wrt_string_buffer{};
   wchar_t* target = nullptr;

   auto result = ::WindowsPreallocateStringBuffer(
       size,
       &target,
       buffer.get_address_of());

    wdl::error::check_com(result);

    swprintf(target, size+1, format, args...);

    auto string = wrt_string{};

    result = ::WindowsPromoteStringBuffer(
        buffer.get(),
        string.get_address_of());

    wdl::error::check_com(result);

    buffer.release();

    return string;
}
