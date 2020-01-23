//
//  main.cpp
//  MCRL
//
//  Created by Arthur Sun on 1/22/20.
//  Copyright © 2020 Arthur Sun. All rights reserved.
//

#include "MCSystem.hpp"

#include <cstdio>

#include <random>
#include <chrono>

std::mt19937 rng(arc4random());

constexpr double decayRate = 0.00001;

double printEvery = 1.0;

auto now() {
    return std::chrono::steady_clock::now();
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type urandom(T a, T b) {
    return std::uniform_int_distribution<T>(a, b)(rng);
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value, T>::type urandom(T a, T b) {
    return std::uniform_real_distribution<T>(a, b)(rng);
}

enum Action : int {
    a_left, a_right, a_die
};

struct State {
    double real;
    double a;

    double state;

    bool dead;
    size_t count;

    State(double a) : real(a), a(std::round(100.0 * a) / 100.0), state(0.0), dead(false), count(0) {
    }

    static constexpr size_t limit = 100;

    std::vector<Action> getActions() const {
        return {a_left, a_right, a_die};
    }

    bool can(const Action &action) const {
        return true;
    }

    void next(const Action &action) {
        constexpr double step = 0.05;

        switch (action) {
            case a_left:
                state -= step;
                break;
            case a_right:
                state += step;
                break;
            case a_die:
                dead = true;
                break;
        }

        ++count;
        dead = dead || count >= limit;
    }
};

struct Hash {
    std::hash<double> hash;

    size_t operator()(const State &a) const {
        return hash(a.a) ^ hash(a.state);
    }
};

struct Equal {
    bool operator()(const State &a, const State &b) const {
        return a.a == b.a && a.state == b.state;
    }
};

using system_t = MCSystem<State, Action, Hash, Equal>;

system_t systemA;

template<class State, class System>
double run(State &state, System &system) {
    size_t steps = 0;
    while (!state.dead) {
        const Action *action = system.getAction(state, 1.0);

        if (action) {
            state.next(*action);
        }

        ++steps;
    }

    double reward = std::cos(10.0 * state.real) - state.state;
    reward = 1.0 - reward * reward;

    system.flush(reward, decayRate);

    return reward;
}

void printMemoryUsage(size_t bytes) {
    printf("Memory usage ");
    if (bytes < pow2(20)) {
        printf("%.3lfKB\n", bytes / pow(2.0, 10.0));
    } else if (bytes < pow2(30)) {
        printf("%.3lfMB\n", bytes / pow(2.0, 20.0));
    } else if (bytes < pow2(40)) {
        printf("%.3lfGB\n", bytes / pow(2.0, 30.0));
    } else if (bytes < pow2(50)) {
        printf("%.3lfTB\n", bytes / pow(2.0, 40.0));
    } else {
        printf("%.3lfPB\n", bytes / pow(2.0, 50.0));
    }
}

int main(int argc, const char *argv[]) {
    size_t i = 0;

    double reward = 0.0;

    auto lastPrint = now();

    while (++i) {
        State state(urandom(-1.0, 1.0));
        reward = (1.0 - decayRate) * reward + decayRate * run(state, systemA);

        auto n = now();

        if(std::chrono::duration<double>(n - lastPrint).count() >= printEvery) {
            lastPrint = n;

            printf("At iteration %zu\n", i);

            printf("Recent average reward: %lf\n", reward);

            size_t size = systemA.set.size();
            printf("Set size %zu\n", size);

            size_t bytes = size * (sizeof(system_t::Node*) + sizeof(system_t::Node));

            printMemoryUsage(bytes);

            printf("\n\n\n");
        }
    }
    return 0;
}
