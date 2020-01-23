//
//  MCSystem.hpp
//  MCRL
//
//  Created by Arthur Sun on 1/22/20.
//  Copyright © 2020 Arthur Sun. All rights reserved.
//

#pragma once

#include <cmath>
#include <stack>
#include <vector>
#include <unordered_set>

constexpr size_t pow2(unsigned p) noexcept {
    return size_t(1) << p;
}

template<typename State, typename Action, typename StateHash, typename StateEqual>
struct MCSystem {

    struct Node {
        State state;
        Action action;

        double reward;
        size_t count;

        bool can;

        Node(const State &a, const Action &b) : state(a), action(b), reward(0.0), count(0) {
        }

        double getRecommendation(size_t parentCount, double explore) {
            return !count ? std::numeric_limits<double>::infinity() : reward + explore * std::sqrt(
                    std::log(parentCount) / count);
        }
    };

    struct Hash {
        StateHash hash;

        size_t operator()(const Node *node) const {
            return hash(node->state);
        }
    };

    struct Equal {
        StateEqual equal;

        size_t operator()(const Node *a, const Node *b) const {
            return equal(a->state, b->state);
        }
    };

    std::stack<Node *> stack;

    std::unordered_multiset<Node *, Hash, Equal> set;

    MCSystem() noexcept {
        set.reserve(pow2(24));
    }

    ~MCSystem() {
        for (Node *node : set) {
            delete node;
        }
    }

    void flush(double reward, double decayRate) {
        while (!stack.empty()) {
            Node *node = stack.top();
            node->reward = ((1.0 - decayRate) * node->reward) + decayRate * reward;
            ++node->count;
            stack.pop();
        }
    }

    const Action *getAction(const State &state, double explore) {
        auto[begin, end] = set.equal_range((Node *) &state);

        Node *choice = nullptr;

        if (begin == end) {
            const std::vector<Action> &actions = state.getActions();

            for (const Action &action : actions) {
                Node *node = new Node(state, action);

                set.emplace(node);

                if (state.can(action) && choice == nullptr) {
                    choice = node;
                }
            }
        } else {
            size_t count = 0;

            for(auto i = begin; i != end; ++i) {
                (*i)->can = state.can((*i)->action);

                if ((*i)->can) {
                    count += (*i)->count;
                }
            }

            double highest = -std::numeric_limits<double>::infinity();

            for (; begin != end; ++begin) {
                if ((*begin)->can) {
                    double r = (*begin)->getRecommendation(count, explore);

                    if (r > highest) {
                        highest = r;
                        choice = *begin;
                    }
                }
            }
        }

        if (choice) {
            stack.push(choice);
            return &choice->action;
        }

        return nullptr;
    }

};
