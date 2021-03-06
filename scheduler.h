#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <algorithm>
#include <array>
#include <cassert>
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

    static bool timed_task_cmp(const Task *a, const Task *b)
    {
        if (a->condition.type != WakeupCondition::TIMER) {
            throw std::logic_error("task a is not timed");
        }
        if (b->condition.type != WakeupCondition::TIMER) {
            throw std::logic_error("task b is not timed");
        }

        return a->condition.wakeup_at > b->condition.wakeup_at;
    }

    using TaskArray = std::array<Task, max_tasks>;
    using TaskPArray = std::array<Task*, max_tasks>;

public:
    Scheduler():
        m_tasks(),
        m_timed_tasks(),
        m_timed_tasks_end(m_timed_tasks.begin()),
        m_event_tasks(),
        m_event_tasks_end(m_event_tasks.begin())
    {

    }

private:
    TaskArray m_tasks;
    TaskPArray m_timed_tasks;
    typename TaskPArray::iterator m_timed_tasks_end;
    TaskPArray m_event_tasks;
    typename TaskPArray::iterator m_event_tasks_end;

private:
    void add_timed_task(Task &task)
    {
        *m_timed_tasks_end++ = &task;
        std::push_heap(m_timed_tasks.begin(), m_timed_tasks_end, &timed_task_cmp);
    }

public:
    template <typename coroutine_t, typename... arg_ts>
    bool add_task(coroutine_t *coro, arg_ts&&... args)
    {
        for (Task &task: m_tasks) {
            if (!task.coro) {
                (*coro)(args...);
                task.coro = coro;
                task.condition.type = WakeupCondition::NONE;
                *m_event_tasks_end++ = &task;
                return true;
            }
        }
        return false;
    }

    void run()
    {
        while (1) {
            sched_clock::time_point now = sched_clock::now();

            while (m_timed_tasks_end != m_timed_tasks.begin()
                   && m_timed_tasks.front()->condition.wakeup_at <= now)
            {
                std::pop_heap(m_timed_tasks.begin(), m_timed_tasks_end);
                Task &task = **(m_timed_tasks_end-1);
                assert(task.condition.type == WakeupCondition::TIMER);
                task.condition = task.coro->step();
                switch (task.condition.type)
                {
                case WakeupCondition::TIMER:
                {
                    // re-add the task to the queue, no need to modify
                    // m_timed_tasks_end
                    std::push_heap(m_timed_tasks.begin(), m_timed_tasks_end);
                    break;
                }
                case WakeupCondition::NONE:
                case WakeupCondition::EVENT:
                {
                    // move task to event list
                    *m_event_tasks_end++ = &task;
                    // intentionall fall-through
                }
                case WakeupCondition::FINSIHED:
                {
                    // remove task
                    m_timed_tasks_end--;
                    break;
                }
                }
            }

            for (auto iter = m_event_tasks.begin();
                 iter != m_event_tasks_end;
                 ++iter)
            {
                Task &task = **iter;
                assert(task.condition.type == WakeupCondition::NONE ||
                       task.condition.type == WakeupCondition::EVENT);
                if (!task.can_run_now(now)) {
                    continue;
                }
                if (task.can_run_now(now)) {
                    task.condition = task.coro->step();
                    switch (task.condition.type)
                    {
                    case WakeupCondition::TIMER:
                    {
                        add_timed_task(task);
                        // intentional fallthrough
                    }
                    case WakeupCondition::FINSIHED:
                    {
                        if (iter != m_event_tasks_end-1) {
                            std::swap(*iter, *(m_event_tasks_end-1));
                        }
                        iter--;
                        m_event_tasks_end--;
                        break;
                    }
                    case WakeupCondition::NONE:;
                    case WakeupCondition::EVENT:;
                    }
                }
            }

            if (m_timed_tasks.begin() != m_timed_tasks_end) {
                // timed task, wait for next task
                sched_clock::time_point next_wakeup = m_timed_tasks.front()->condition.wakeup_at;
                int64_t us_to_sleep = std::chrono::duration_cast<std::chrono::microseconds>(next_wakeup - now).count();
                if (us_to_sleep > 0) {
                    usleep(us_to_sleep);
                }
            }
        }
    }

};

#endif // SCHEDULER_H
