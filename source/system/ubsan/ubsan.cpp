// Copyright (C) 2021-2022  ilobilo

#include <system/ubsan/ubsan.hpp>
#include <lib/string.hpp>
#include <lib/log.hpp>

static void print(const char *message, source_location loc)
{
    if (strstr(loc.file, "acpi.cpp") && ((loc.line == 101 && loc.column == 55) || (loc.line == 100 && loc.column == 64))) return;
    warn("Ubsan: %s at file %s, line %d, column %d", message, loc.file, loc.line, loc.column);
    while (true) asm volatile ("cli; hlt");
}

extern "C" void __ubsan_handle_add_overflow(overflow_data *data)
{
    print("addition overflow", data->location);
}

extern "C" void __ubsan_handle_sub_overflow(overflow_data *data)
{
    print("subtraction overflow", data->location);
}

extern "C" void __ubsan_handle_mul_overflow(overflow_data *data)
{
    print("multiplication overflow", data->location);
}

extern "C" void __ubsan_handle_divrem_overflow(overflow_data *data)
{
    print("division overflow", data->location);
}

extern "C" void __ubsan_handle_negate_overflow(overflow_data *data)
{
    print("negation overflow", data->location);
}

extern "C" void __ubsan_handle_pointer_overflow(overflow_data *data)
{
    print("pointer overflow", data->location);
}

extern "C" void __ubsan_handle_shift_out_of_bounds(shift_out_of_bounds_data *data)
{
    print("shift out of bounds", data->location);
}

extern "C" void __ubsan_handle_load_invalid_value(invalid_value_data *data)
{
    print("invalid load value", data->location);
}

extern "C" void __ubsan_handle_out_of_bounds(array_out_of_bounds_data *data)
{
    print("array out of bounds", data->location);
}

extern "C" void __ubsan_handle_type_mismatch_v1(type_mismatch_v1_data *data, uintptr_t ptr)
{
    if (ptr == 0) print("use of NULL pointer", data->location);

    else if (ptr & ((1 << data->log_alignment) - 1))
    {
        print("use of misaligned pointer", data->location);
    }
    else print("no space for object", data->location);
}

extern "C" void __ubsan_handle_vla_bound_not_positive(negative_vla_data *data)
{
    print("variable-length argument is negative", data->location);
}

extern "C" void __ubsan_handle_nonnull_return(nonnull_return_data *data)
{
    print("non-null return is null", data->location);
}

extern "C" void __ubsan_handle_nonnull_return_v1(nonnull_return_data *data)
{
    print("non-null return is null", data->location);
}

extern "C" void __ubsan_handle_nonnull_arg(nonnull_arg_data *data)
{
    print("non-null argument is null", data->location);
}

extern "C" void __ubsan_handle_builtin_unreachable(unreachable_data *data)
{
    print("unreachable code reached", data->location);
}

extern "C" void __ubsan_handle_invalid_builtin(invalid_builtin_data *data)
{
    print("invalid builtin", data->location);
}

extern "C" void __ubsan_handle_float_cast_overflow(float_cast_overflow_data *data)
{
    print("float cast overflow", data->location);
}

extern "C" void __ubsan_handle_missing_return(missing_return_data *data)
{
    print("missing return", data->location);
}

extern "C" void __ubsan_handle_alignment_assumption(alignment_assumption_data *data)
{
    print("alignment assumption", data->location);
}