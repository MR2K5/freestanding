#pragma once

#include <__algorithm/nonmodifying.hpp>
#include <__ranges/core.hpp>
#include <optional>

namespace std::ranges {
template<view V, class Pred>
requires input_range<V> && is_object_v<Pred> && indirect_unary_predicate<Pred const, iterator_t<V>>
class drop_while_view: public view_interface<drop_while_view<V, Pred>> {
public:
    drop_while_view() requires default_initializable<V> && default_initializable<Pred> = default;
    constexpr explicit drop_while_view(V base, Pred pred)
        : base_(std::move(base)), pred_(std::move(pred)) {}

    constexpr V base() const& requires copy_constructible<V> { return base_; }
    constexpr V base() && { return std::move(base_); }

    constexpr Pred const& pred() const { return *pred_; }

    constexpr auto begin() {
        if constexpr (forward_range<V>) {
            if (!cache_) cache_ = find_if_not(base_, std::cref(*pred_));
            return *cache_;
        } else {
            return find_if_not(base_, std::cref(*pred_));
        }
    }

    constexpr auto end() { return ranges::end(base_); }

private:
    [[no_unique_address]] V base_ = V();
    [[no_unique_address]] __detail::movable_box<Pred> pred_;
    [[no_unique_address]] conditional_t<
        forward_range<V>, __detail::non_propagating_cache<iterator_t<V>>, __empty> cache_;
};

template<class T, class Pred>
constexpr bool enable_borrowed_range<drop_while_view<T, Pred>> = enable_borrowed_range<T>;

template<class R, class Pred> drop_while_view(R&&, Pred) -> drop_while_view<views::all_t<R>, Pred>;

namespace views {
inline constexpr struct __drop_while_fn {
    static constexpr auto operator()(auto&& E, auto&& F)
        requires requires { drop_while_view(FWD(E), FWD(F)); } {
        return drop_while_view(FWD(E), FWD(F));
    }
    static constexpr auto operator()(auto&& F) {
        return __range_adaptor_closure_fn(std::bind_back<__drop_while_fn>(FWD(F)));
    }
} drop_while;
}  // namespace views

}  // namespace std::ranges
