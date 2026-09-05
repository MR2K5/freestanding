#pragma once

#include <__algorithm/nonmodifying.hpp>
#include <__functional/core.hpp>

namespace std {
    
template<class ForwardIterator1, class BinaryPredicate = equal_to<>> class default_searcher {
public:
    constexpr default_searcher(
        ForwardIterator1 pat_first, ForwardIterator1 pat_last,
        BinaryPredicate pred = BinaryPredicate()
    )
        : pat_first_(pat_first), pat_last_(pat_last), pred_(pred) {}

    template<class ForwardIterator2>
    constexpr pair<ForwardIterator2, ForwardIterator2>
    operator()(ForwardIterator2 first, ForwardIterator2 last) const {
        auto [match_begin, match_end] = ranges::search(first, last, pat_first_, pat_last_, pred_);
        return {match_begin, match_end};
    }

private:
    ForwardIterator1 pat_first_;
    ForwardIterator1 pat_last_;
    BinaryPredicate pred_;
};

// Deduction Guide
template<class ForwardIterator, class BinaryPredicate = equal_to<>>
default_searcher(ForwardIterator, ForwardIterator, BinaryPredicate = BinaryPredicate())
    -> default_searcher<ForwardIterator, BinaryPredicate>;

}  // namespace std
