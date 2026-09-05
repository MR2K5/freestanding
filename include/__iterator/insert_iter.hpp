#pragma once

#include <__ranges/core.hpp>

namespace std {

template<class Container> class back_insert_iterator {
protected:
    Container* container;

public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = ptrdiff_t;
    using pointer           = void;
    using reference         = void;
    using container_type    = Container;

    constexpr explicit back_insert_iterator(Container& x) noexcept: container(std::addressof(x)) {}
    constexpr back_insert_iterator& operator=(typename Container::value_type const& value) {
        container->push_back(value);
        return *this;
    }
    constexpr back_insert_iterator& operator=(typename Container::value_type&& value) {
        container->push_back(std::move(value));
        return *this;
    }

    constexpr back_insert_iterator& operator*() noexcept { return *this; }
    constexpr back_insert_iterator& operator++() noexcept { return *this; }
    constexpr back_insert_iterator operator++(int) noexcept { return *this; }
};

template<class Container> constexpr back_insert_iterator<Container> back_inserter(Container& x) {
    return back_insert_iterator<Container>(x);
}

template<class Container> class front_insert_iterator {
protected:
    Container* container;

public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = ptrdiff_t;
    using pointer           = void;
    using reference         = void;
    using container_type    = Container;

    constexpr explicit front_insert_iterator(Container& x) noexcept: container(std::addressof(x)) {}
    constexpr front_insert_iterator& operator=(typename Container::value_type const& value) {
        container->push_front(value);
        return *this;
    }
    constexpr front_insert_iterator& operator=(typename Container::value_type&& value) {
        container->push_front(std::move(value));
        return *this;
    }

    constexpr front_insert_iterator& operator*() noexcept { return *this; }
    constexpr front_insert_iterator& operator++() noexcept { return *this; }
    constexpr front_insert_iterator operator++(int) noexcept { return *this; }
};

template<class Container> constexpr front_insert_iterator<Container> front_inserter(Container& x) {
    return front_insert_iterator<Container>(x);
}

template<class Container> class insert_iterator {
protected:
    Container* container;
    ranges::iterator_t<Container> iter;

public:
    using iterator_category = output_iterator_tag;
    using value_type        = void;
    using difference_type   = ptrdiff_t;
    using pointer           = void;
    using reference         = void;
    using container_type    = Container;

    constexpr insert_iterator(Container& x, ranges::iterator_t<Container> i)
        : container(std::addressof(x)), iter(i) {}
    constexpr insert_iterator& operator=(typename Container::value_type const& value) {
        iter = container->insert(iter, value);
        ++iter;
        return *this;
    }
    constexpr insert_iterator& operator=(typename Container::value_type&& value) {
        iter = container->insert(iter, std::move(value));
        ++iter;
        return *this;
    }

    constexpr insert_iterator& operator*() noexcept { return *this; }
    constexpr insert_iterator& operator++() noexcept { return *this; }
    constexpr insert_iterator& operator++(int) noexcept { return *this; }
};

template<class Container>
  constexpr insert_iterator<Container>
    inserter(Container& x, ranges::iterator_t<Container> i) {
        return insert_iterator<Container>(x, i);
    }

}  // namespace std
