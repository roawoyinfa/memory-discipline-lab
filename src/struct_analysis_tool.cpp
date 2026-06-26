#include <array>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>

struct MemberDescriptor {
    std::string_view name;
    std::size_t offset;
    std::size_t size;
    std::size_t alignment;
};

template <typename T>
void analyze_struct(
    std::string_view name,
    std::span<const MemberDescriptor> members)
{
    constexpr std::string_view kIndent{"         "};

    std::cout << "\nStruct " << name << '\n';
    std::cout << "==================================================\n\n";

    const std::size_t struct_size{sizeof(T)};
    const std::size_t struct_alignment{alignof(T)};

    std::cout << "Size      : " << struct_size << " bytes" << '\n';
    std::cout << "Alignment : " << struct_alignment << " bytes" << '\n';

    std::cout << "\nMemory Layout\n\n";

    std::size_t total_internal_padding{};

    // ---------------------------------------------------------
    // Offset row
    // ---------------------------------------------------------

    std::cout << "  Offset ";

    std::size_t next_offset{};

    for (const auto& member : members) {
        const std::size_t padding_before{member.offset - next_offset};

        const std::size_t cell_width{member.size * 2 + member.name.size()};

        if (padding_before > 0) {
            total_internal_padding += padding_before;
            std::cout << next_offset << std::string(padding_before * 2, ' ');
        }

        std::cout << member.offset;
        std::cout << std::string(cell_width, ' ');

        next_offset = member.offset + member.size;
    }

    const std::size_t trailing_padding{struct_size - next_offset};
    if (trailing_padding > 0) {
        std::cout << next_offset << std::string(trailing_padding * 2, ' ');
    }

    std::cout << struct_size << '\n';

    // ---------------------------------------------------------
    // Top border
    // ---------------------------------------------------------

    std::cout << kIndent;

    next_offset = 0;

    for (const auto& member : members) {
        const std::size_t padding_before{member.offset - next_offset};

        const std::size_t cell_width{member.size * 2 + member.name.size()};

        if (padding_before > 0) {
            std::cout << "+" << std::string(padding_before * 2, '-');
        }

        std::cout << "+" << std::string(cell_width, '-');

        next_offset = member.offset + member.size;
    }

    if (trailing_padding > 0) {
        std::cout << "+" << std::string(trailing_padding * 2, '-');
    }

    std::cout << "+\n";

    // ---------------------------------------------------------
    // Contents
    // ---------------------------------------------------------

    std::cout << "Contents ";

    next_offset = 0;

    for (const auto& member : members) {
        const std::size_t padding_before{member.offset - next_offset};

        if (padding_before > 0) {
            std::cout << '|' << std::string(padding_before * 2, ' ');
        }

        std::cout << "|";
        std::cout << std::string(member.size, ' ');
        std::cout << member.name;
        std::cout << std::string(member.size, ' ');

        next_offset = member.offset + member.size;
    }

    if (trailing_padding > 0) {
        std::cout << "|" << std::string(trailing_padding * 2, ' ');
    }

    std::cout << "|\n";

    // ---------------------------------------------------------
    // Bottom border
    // ---------------------------------------------------------

    std::cout << kIndent;

    next_offset = 0;

    for (const auto& member : members) {
        const std::size_t padding_before{member.offset - next_offset};

        const std::size_t cell_width{member.size * 2 + member.name.size()};

        if (padding_before > 0) {
            std::cout << "+" << std::string(padding_before * 2, '_');
        }

        std::cout << "+" << std::string(cell_width, '_');

        next_offset = member.size + member.offset;
    }

    if (trailing_padding > 0) {
        std::cout << "+" << std::string(trailing_padding * 2, '_');
    }

    std::cout << "+\n";

    std::cout << "\n\nEmpty cells represent compiler-inserted padding\n";

    // ---------------------------------------------------------
    // Members Information
    // ---------------------------------------------------------

    std::cout << "\nMembers\n";
    std::cout << "--------------------------------------------------\n";

    std::cout << "Name    Offset   Size   Alignment\n";
    std::cout << "----    ------   ----   ---------\n";

    for (const auto& member : members) {
        std::cout
            << std::left
            << std::setw(12) << member.name
            << std::setw(8)  << member.offset
            << std::setw(8)  << member.size
            << member.alignment
            << '\n';
    }

    // ---------------------------------------------------------
    // Padding Information
    // ---------------------------------------------------------

    std::cout << "\nPadding\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << "Internal Padding : " << total_internal_padding << " bytes\n";
    std::cout << "Trailing Padding : " << trailing_padding << " bytes\n";
    std::cout
        << "Total Padding    : "
        << total_internal_padding + trailing_padding
        << " bytes\n";
};

constexpr MemberDescriptor member(
    std::string_view name,
    std::size_t offset,
    std::size_t size,
    std::size_t alignment)
{
    return MemberDescriptor{name, offset, size, alignment};
}

#define MEMBER(T, M)                                      \
    member(                                               \
        #M,                                               \
        offsetof(T, M),                                   \
        sizeof(decltype(std::declval<T>().M)),            \
        alignof(decltype(std::declval<T>().M)))

int main()
{
    std::cout << "=== Struct Analysis Tool ===\n\n";

    // ---------------------------------------------------------
    // Internal padding
    // ---------------------------------------------------------

    struct InternalPadding {
        int  i;
        char c;
        int  j;
    };

    analyze_struct<InternalPadding>(
        "InternalPadding",
        std::array<MemberDescriptor, 3>{
            MEMBER(InternalPadding, i),
            MEMBER(InternalPadding, c),
            MEMBER(InternalPadding, j),
        });


    static_assert(sizeof(InternalPadding) == 12);
    static_assert(offsetof(InternalPadding, i) == 0);
    static_assert(offsetof(InternalPadding, c) == 4);
    static_assert(offsetof(InternalPadding, j) == 8);

    // ---------------------------------------------------------
    // Trailing padding
    // ---------------------------------------------------------

    struct TrailingPadding {
        int  i;
        char c;
    };

    analyze_struct<TrailingPadding>(
        "TrailingPadding",
        std::array<MemberDescriptor, 2>{
            MEMBER(TrailingPadding, i),
            MEMBER(TrailingPadding, c),
        });

    static_assert(sizeof(TrailingPadding) == 8);
    static_assert(offsetof(TrailingPadding, i) == 0);
    static_assert(offsetof(TrailingPadding, c) == 4);

    std::cout << "\nAll tests passed.\n";
}
