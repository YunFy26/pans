#include "pans/macros.h"
#include <iostream>
#include <limits>
#include <type_traits>   // 在编译器获取、判断、修改类型

// 编译期间检查
static_assert(sizeof(u8) == 1 && std::is_unsigned_v<u8>);
static_assert(sizeof(s8) == 1 && std::is_signed_v<s8>);
static_assert(sizeof(u16) == 2 && std::is_unsigned_v<u16>);
static_assert(sizeof(s16) == 2 && std::is_signed_v<s16>);
static_assert(sizeof(u32) == 4 && std::is_unsigned_v<u32>);
static_assert(sizeof(s32) == 4 && std::is_signed_v<s32>);
static_assert(sizeof(u64) == 8 && std::is_unsigned_v<u64>);
static_assert(sizeof(s64) == 8 && std::is_signed_v<s64>);
static_assert(INVALID8 == MAX_U8);
static_assert(INVALID16 == MAX_U16);
static_assert(INVALID32 == MAX_U32);
static_assert(INVALID64 == MAX_U64);
static_assert(MAX_U8 == std::numeric_limits<u8>::max());
static_assert(MAX_U16 == std::numeric_limits<u16>::max());
static_assert(MAX_U32 == std::numeric_limits<u32>::max());
static_assert(MAX_U64 == std::numeric_limits<u64>::max());

// 运行时检查
void test_types_and_constants()
{
    ASSERT_NOEFFECT(sizeof(u8) == 1 && std::is_unsigned_v<u8>);
    ASSERT_NOEFFECT(sizeof(s8) == 1 && std::is_signed_v<s8>);
    ASSERT_NOEFFECT(sizeof(u16) == 2 && std::is_unsigned_v<u16>);
    ASSERT_NOEFFECT(sizeof(s16) == 2 && std::is_signed_v<s16>);
    ASSERT_NOEFFECT(sizeof(u32) == 4 && std::is_unsigned_v<u32>);
    ASSERT_NOEFFECT(sizeof(s32) == 4 && std::is_signed_v<s32>);
    ASSERT_NOEFFECT(sizeof(u64) == 8 && std::is_unsigned_v<u64>);
    ASSERT_NOEFFECT(sizeof(s64) == 8 && std::is_signed_v<s64>);
    ASSERT_NOEFFECT(INVALID8 == MAX_U8);
    ASSERT_NOEFFECT(INVALID16 == MAX_U16);
    ASSERT_NOEFFECT(INVALID32 == MAX_U32);
    ASSERT_NOEFFECT(INVALID64 == MAX_U64);
    ASSERT_RETNONE(MAX_U8 == std::numeric_limits<u8>::max());
    ASSERT_RETNONE(MAX_U16 == std::numeric_limits<u16>::max());
    ASSERT_RETNONE_INFO(MAX_U32 == std::numeric_limits<u32>::max(), "This is a test for ASSERT_NOEFFECT_INFO macro.");
    ASSERT_RETNONE_INFO(MAX_U64 == std::numeric_limits<u64>::max(), "This is a test for ASSERT_NOEFFECT_INFO macro.");
}

// [[nodiscard]] 必须检查返回值
[[nodiscard]] int test_assert_retval()
{
    ASSERT_RETVAL(false, -1);
    return 0;
}

[[nodiscard]] int test_assert_retval_info()
{
    ASSERT_RETVAL_INFO(false, -2, "This should not trigger an assertion.");
    return 0;
}

void test_assert_macros()
{
    ASSERT_NOEFFECT_INFO(true, "This should not trigger an assertion.");
    int never_reached_condition = 5;
    int i = 0;
    for (i = 0; i < 10; i++)
    {
        ASSERT_CONTINUE(i != never_reached_condition);
        std::cout << "ASSERT_CONTINUE " << i << std::endl;
    }
    std::cout << "A-------------------------------------------------------:" << i << std::endl;
    std::cout << "" << std::endl;
    
    for (i = 0; i < 10; i++)
    {
        ASSERT_CONTINUE_INFO(i != never_reached_condition, "loop index should be " << i);
    }
    std::cout << "B-------------------------------------------------------:" << i << std::endl;
    std::cout << "" << std::endl;
    
    for (i = 0; i < 10; i++)
    {
        ASSERT_BREAK(i != never_reached_condition);
        std::cout << "ASSERT_BREAK " << i << std::endl;
    }
    std::cout << "C-------------------------------------------------------:" << i << std::endl;
    std::cout << "" << std::endl;
    
    for (i = 0; i < 10; i++)
    {
        ASSERT_BREAK_INFO(i != never_reached_condition, "loop index should be " << i);
    }
    std::cout << "D-------------------------------------------------------:" << i << std::endl;
    std::cout << "" << std::endl;
    
    for (i = 0; i < 10; i++)
    {
        ASSERT_NOEFFECT(i != never_reached_condition);
        std::cout << "ASSERT_NOEFFECT " << i << std::endl;
    }
    std::cout << "E-------------------------------------------------------:" << i << std::endl;
    std::cout << "" << std::endl;
    
    for (i = 0; i < 10; i++)
    {
        ASSERT_NOEFFECT_INFO(i != never_reached_condition, "loop index should be " << i);
    }
    std::cout << "F-------------------------------------------------------:" << i << std::endl;   
    std::cout << "" << std::endl;

}

int main()
{
#ifdef PANS_DEBUG
    std::cout << "This program is compiled in debug mode. Assert will terminate the program on failure." << std::endl;
#endif

#ifdef NODEBUG
    std::cout << "This program is compiled in release mode. Assert will do nothing on failure." << std::endl;
#endif

    test_types_and_constants();
    test_assert_macros();

    auto retval_1 = test_assert_retval();
    std::cout << "test_assert_retval() returned: " << retval_1 << std::endl;
    auto retval_2 = test_assert_retval_info();
    std::cout << "test_assert_retval() returned: " << retval_2 << std::endl;

    return EXIT_SUCCESS;

}