#pragma once
#ifndef EXPANDABLE_ARRAY_H_
#define EXPANDABLE_ARRAY_H_
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <utility>
#include <type_traits>

namespace expandable_array {
    template <typename...>
    using voidType = void;

    template <typename T, typename = void>
    struct IsIterator : public std::false_type {};

    template <typename T>
    struct IsIterator<T, voidType<typename T::iterator_category>> : public std::true_type {};

    template <typename T>
    struct IsIterator<T, std::enable_if_t<std::is_pointer_v<T>>> : public std::true_type {};

    template <typename T>
    constexpr bool is_iterator_value = IsIterator<T>::value;

    template <typename T, typename Alloc = std::allocator<T>>
    class ExpandableArray
    {
    public:
        using allocator_type = Alloc;
        using allocator_traits_type = std::allocator_traits<Alloc>;
        using size_type = typename allocator_traits_type::size_type;
        using difference_type = typename allocator_traits_type::difference_type;

        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
private:
        static constexpr std::size_t default_capacity = 4;

        template <typename ValueType, std::enable_if_t<std::is_array_v<ValueType>, int> = 0>
        void destroy_range(const ValueType* begin, const ValueType* end)
        {
            using std::extent_v;

            while (begin != end) {
                destroy_range(*begin, *begin + extent_v<T>);
                allocator_traits_type::destroy(m_allocator, begin++);
            }
        }

        template <typename ValueType, std::enable_if_t<!std::is_array_v<ValueType>, int> = 0>
        void destroy_range(const ValueType* begin, const ValueType* end)
        {
            while (begin != end) {
                allocator_traits_type::destroy(m_allocator, begin++);
            }
        }

        void destroy_all() noexcept
        {
            destroy_range(m_array.first, m_array.second);
        }
public:
        explicit ExpandableArray(const allocator_type& allocator = allocator_type{})
            : m_allocator(allocator)
            , m_array(allocator_traits_type::allocate(m_allocator, default_capacity), nullptr)
            , m_capacity(default_capacity)
        {
            m_array.second = m_array.first;
        }

        explicit ExpandableArray(
            size_type size,
            const_reference value = value_type{},
            const allocator_type& allocator = allocator_type{}
        )
            : m_allocator(allocator)
            , m_array(allocator_traits_type::allocate(m_allocator, size * 2), nullptr)
            , m_capacity(size * 2)
        {
            using std::uninitialized_fill;

            m_array.second = m_array.first + size;
            uninitialized_fill(m_array.first, m_array.second, value);
        }

        template <typename InIter, std::enable_if_t<is_iterator_value<InIter>,
            int> = 0>
        ExpandableArray(
            InIter begin,
            InIter end,
            const allocator_type& allocator = allocator_type{}
        )
            : m_allocator(allocator)
            , m_array(allocator_traits_type::allocate(m_allocator, std::distance(begin, end) * 2), nullptr)
            , m_capacity(std::distance(begin, end))
        {
            using std::uninitialized_copy;

            m_array.second = uninitialized_copy(begin, end, m_array.first);
        }

        ExpandableArray(const ExpandableArray& right)
            : ExpandableArray(
                right.m_array.first,
                right.m_array.second,
                right.m_allocator
            ) {}
        
        ExpandableArray(ExpandableArray&& right) noexcept
            : m_allocator(std::move(right.m_allocator))
            , m_array(std::move(right.m_array))
            , m_capacity(std::move(right.m_capacity))
        {
            right.m_array = { nullptr, nullptr };
        }

        ExpandableArray(
            std::initializer_list<value_type> initializer_list,
            const allocator_type& allocator = allocator_type{}
        )
            : m_allocator(allocator)
            , m_array(allocator_traits_type::allocate(m_allocator, initializer_list.size() * 2), nullptr)
            , m_capacity(initializer_list.size() * 2)
        {
            using std::uninitialized_copy;

            m_array.second = uninitialized_copy(
                initializer_list.begin(),
                initializer_list.end(),
                m_array.first
            );
        }

        ~ExpandableArray()
        {
            pointer iter = m_array.first;
            while (iter != m_array.second) {
                allocator_traits_type::destroy(m_allocator, iter++);
            }

            allocator_traits_type::deallocate(m_allocator, m_array.first, m_capacity);
        }

        ExpandableArray& operator=(const ExpandableArray& right)
        {
            using std::copy;

            if (right.size() > size()) {
                resize(right.size());
            }

            m_array.second = copy(right.m_array.first, right.m_array.second, m_array.first);
        }

        ExpandableArray& operator=(ExpandableArray&& right) noexcept
        {
            using std::move;

            m_array = move(right.m_array);
            right.m_array = { nullptr, nullptr };
            m_capacity = right.m_capacity;
        }

        size_type size() const noexcept
        {
            return m_array.second - m_array.first;
        }

        void reserve(size_type new_capacity)
        {
            using std::min;
            using std::move;
            using std::uninitialized_copy_n;

            std::pair<pointer, pointer> new_array(
                allocator_traits_type::allocate(m_allocator, new_capacity),
                nullptr
            );

            new_array.second = uninitialized_copy_n(
                    m_array.first,
                    min(
                        static_cast<size_type>(m_array.second - m_array.first), new_capacity
                    ),
                    new_array.first
            );

            destroy_all();
            allocator_traits_type::deallocate(m_allocator, m_array.first, m_capacity);
            m_capacity = new_capacity;
            m_array = move(new_array);
        }

        void resize(size_type new_size, const_reference value = value_type{})
        {
            using std::uninitialized_fill_n;

            reserve(new_size * 2);
            if (new_size > size()) {
                uninitialized_fill_n(m_array.first + size(), new_size - size(), value);
            } else {
                destroy_range(m_array.first + size() - new_size, m_array.second);
            }
        }
    private:
        allocator_type m_allocator;
        std::pair<pointer, pointer> m_array;
        size_type m_capacity;
    };
}
#endif // EXPANDABLE_ARRAY_H_
