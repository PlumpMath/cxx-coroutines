#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <thread>

#include <unistd.h>

#include "coroutine.h"

template <std::size_t max_tasks>
class Scheduler
{
private:
    struct Task
    {
        Task():
            coro(nullptr)
        {

        }

        WakeupCondition condition;
        Coroutine *coro;

        inline bool is_dead() const
        {
            return !coro || condition.type == WakeupCondition::FINSIHED;
        }

        inline bool can_run_now(const sched_clock::time_point now) const
        {
            switch (condition.type) {
            case WakeupCondition::NONE:
                return true;
            case WakeupCondition::FINSIHED:
                return false;
            case WakeupCondition::TIMER:
                return condition.wakeup_at <= now;
            case WakeupCondition::EVENT:
                return *condition.event_bits == 0;
            }
        }
    };

public:
    Scheduler():
        m_tasks()
    {

    }

private:
    std::array<Task, max_tasks> m_tasks;

public:
    template <typename coroutine_t, typename... arg_ts>
    bool add_task(coroutine_t *coro, arg_ts&&... args)
    {
        for (Task &task: m_tasks) {
            if (!task.coro) {
                (*coro)(args...);
                task.coro = coro;
                task.condition.type = WakeupCondition::NONE;
                return true;
            }
        }
        return false;
    }

    void run()
    {
        while (1) {
            sched_clock::time_point now = sched_clock::now();
            auto cmp = [now](const Task &a, const Task &b) {
                // always order live coroutines before dead ones
                if (a.is_dead()) {
                    return false;
                } else if (b.is_dead()) {
                    return true;
                }

                const bool a_runnable = a.can_run_now(now);
                const bool b_runnable = b.can_run_now(now);

                // order coroutines due for wakeup now before others
                if (b_runnable) {
                    return false;
                } else if (a_runnable) {
                    return true;
                }

                const WakeupCondition::WakeupConditionType ta = a.condition.type;
                const WakeupCondition::WakeupConditionType tb = b.condition.type;

                if (ta == WakeupCondition::TIMER && tb == WakeupCondition::TIMER) {
                    return a.condition.wakeup_at < b.condition.wakeup_at;
                }

                // if "everything else" is not runnable, timers must sort
                // before everything else
                if (ta == WakeupCondition::TIMER && tb != WakeupCondition::TIMER) {
                    return true;
                }

                return false;
            };

            std::sort(
                        m_tasks.begin(),
                        m_tasks.end(),
                        cmp);

            Task &first = m_tasks[0];
            if (first.is_dead()) {
                return;
            }

            if (first.condition.type == WakeupCondition::TIMER) {
                sched_clock::time_point next_wakeup = first.condition.wakeup_at;
                if (next_wakeup > now) {
                    auto dt = next_wakeup - now;
                    std::cout << "sleeping for " << std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() << " ms" << std::endl;
                    usleep(std::chrono::duration_cast<std::chrono::microseconds>(dt).count());
                    auto new_now = sched_clock::now();
                    std::cout << "woke up from sleep after " << std::chrono::duration_cast<std::chrono::milliseconds>(new_now - now).count() << " ms" << std::endl;
                    continue;
                }
            }

            for (Task &task: m_tasks) {
                if (task.is_dead()) {
                    break;
                }
                if (task.can_run_now(now)) {
                    task.condition = task.coro->step();
                }
            }
        }
    }

};

#endif // SCHEDULER_H
