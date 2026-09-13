#ifndef PANS_SRC_LOGGER_BUFFER_H
#define PANS_SRC_LOGGER_BUFFER_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <streambuf>
#include <string_view>
#include <vector>

/**
 * @file buffer.h
 * @brief 定义日志缓冲区
 */
namespace pans::detail{

/**
 * @brief char 缓冲区
 * 
 * 使用 char 存储内容，避免每次都使用 std::string 产生堆分配
 * 短内容存储在内联数组中，长内容转而切换到堆上分配的vector中
 * TODO:一定在堆上吗
 * 
 * @tparam INLINE_CAPACITY 内联缓冲区数组长度
 */
template<std::size_t INLINE_CAPACITY>
class InlineBuffer
{
public:
    /**
     * @brief 追加一段内容
     * 
     * @param data 源数据其实地址
     * @param size 要追加的长度
     */
    void append(const char* data, std::size_t size)
    {
        if (size == 0)
        {
            return;
        }
        // 写入短日志缓冲区
        if (m_overflow.empty() && m_size + size <= INLINE_CAPACITY)
        {
            std::memcpy(m_inline.data() + m_size, data, size);
            m_size += size;
            return;
        }
        // 写入长日志缓冲区
        if (m_overflow.empty())     // 首次溢出
        {
            const std::size_t required_capacity = m_size + size;
            m_overflow.reserve(std::max(INLINE_CAPACITY * 2, required_capacity));
            // 先把 m_inline 中的数据搬到 m_overflow 中
            m_overflow.insert(m_overflow.end(), m_inline.data(), m_inline.data() + m_size); // 起始地址，结束地址
        }

        // m_overflow 已有内容，追加到其后
        m_overflow.insert(m_overflow.end(), data, data + size);
        m_size = m_overflow.size();
        
    }

    /**
     * @brief 追加一个 string_view 指向的内容
     */
    void append(std::string_view value)
    {
        append(value.data(), value.size());
    }

    /**
     * @brief 追加单个字符
     */
    void append(char value)
    {
        append(&value, 1);
    }

    /**
     * @brief 返回当前内容的起始地址
     */
    [[nodiscard]] const char* data() const noexcept
    {
        return m_overflow.empty() ? m_inline.data() : m_overflow.data();
    }

    /**
     * @brief 返回已写的字节数
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_size;
    }

    /**
     * @brief 返回指向当前内容的只读视图
     */
    [[nodiscard]] std::string_view view() const noexcept
    {
        return {data(), size()};
    }
private:
    std::array<char, INLINE_CAPACITY> m_inline{};       // 短日志
    std::vector<char> m_overflow;                       // 长日志
    std::size_t m_size = 0;
};

/**
 * @brief 流式缓冲区适配器（可以使用 std::ostream 向 InlineBuffer 中写入内容）
 *
 * @code{.cpp}
 * pans::detail::InlineBuffer<128> buffer;
 * pans::detail::SmallStreamBuffer<128> sb(buffer);
 * std::ostream os(&sb);
 *
 * os.write("hello", 5);        // 多字符
 * os.put('\n');                // 单字符
 * os << "INFO " << 42;         // os << 4 会先把 4 转位 ‘4’ 再写入
 *
 * buffer.view();
 * @endcode
 *
 * 调用链（公开的非虚接口 → 基类的虚函数 → 派生类的非虚实现）：
 * @code
 * os.write(p, n)
 *   → std::ostream::write
 *     → std::streambuf::sputn
 *       → SmallStreamBuffer::xsputn
 *         → InlineBuffer::append(p, n)
 *
 * os.put(c)
 *   → std::ostream::put
 *     → std::streambuf::sputc                  TODO:streambuf 中 sputc 的实现不一定调用 overflow，这里暂时不考虑
 *       → SmallStreamBuffer::overflow          
 *         → InlineBuffer::append(c)
 *
 * os << x
 *   → std::ostream::operator<<                 // 先格式化 x
 *     → 再走上面两条链之一
 * @endcode
 *
 * @tparam INLINE_CAPACITY 传给所绑定 InlineBuffer 的内联容量，需与其实参一致。
 */
template<std::size_t INLINE_CAPACITY>
class SmallStreamBuffer final : public std::streambuf
{
public:
    // explicit 禁止隐式转换
    explicit SmallStreamBuffer(InlineBuffer<INLINE_CAPACITY>& buffer) noexcept
        : m_buffer(buffer)
    {
    }

protected:
     /**
       * @brief 批量写入入口（put n characters）。
       *
       * os.write()、os << 字符串/数值等操作会走这里，把一批连续字符一次性
       * 转交给 InlineBuffer，避免逐字符调用带来的开销。
       *
       * @param data 待写入字符的起始地址。
       * @param size 待写入的字符数。
       * @return 实际写入的字符数；本实现不会失败，故等于 size。
       */
    std::streamsize xsputn(const char* data, std::streamsize size) override
    {
        if (size <= 0)
        {
            return 0;   
        }
        m_buffer.append(data, static_cast<std::size_t>(size));
        return size;
    }

    /**
       * @brief 单字符写入入口，对应 std::streambuf 的受保护虚函数（虚函数分派点）。
       *
       * 两种调用方式：
       *   - character != eof()：写入该字符，并返回其值表示成功；
       *   - character == eof()：本次没有字符要写。按 overflow 的约定，此时应交付
       *     所有挂起输出，并返回非 eof 值表示成功。
       *
       * 挂起输出指已由 SmallStreamBuffer 接收、但尚未写入 InlineBuffer 的字符。
       * 本实现的交付窗口长度为 0，见 xsputn：
       *
       * @code{.cpp}
       * std::streamsize SmallStreamBuffer::xsputn(const char* data, std::streamsize size) override
       * {
       *     // 进入本函数时：这 size 个字节已被 streambuf 接收，尚未进入 InlineBuffer
       *     m_buffer.append(data, static_cast<std::size_t>(size));
       *     // 返回之后：已交付完毕，窗口关闭
       *     return size;
       * }
       * @endcode
       *
       * 即 append 返回时字符已位于 InlineBuffer，挂起输出恒为空。因此 eof 分支
       * 无需任何交付动作，只需返回成功状态；返回 eof 则会使上层流置位 badbit。
       *
       * @param character 待写入字符的 int_type 表示，或以 eof() 表示无字符调用。
       * @return 非 eof 表示成功；eof 表示失败。
       */
    int_type overflow(int_type character) override
    {
        if (traits_type::eq_int_type(character, traits_type::eof()))
        {
            // 无字符调用：本类无挂起输出可交付，返回非 eof 值表示成功
            return traits_type::not_eof(character);
        }

        m_buffer.append(traits_type::to_char_type(character));
        return character;   // 已同步写入 InlineBuffer，原样返回表示成功
    }

private:
    InlineBuffer<INLINE_CAPACITY>& m_buffer;
};

    
} // namespace pans::detail


#endif