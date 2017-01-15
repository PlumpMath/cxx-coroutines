#include <coroutine.h>

#include <iostream>


static volatile uint8_t sigint_ev = 1;


Coroutine::Coroutine():
    m_state_line(0)
{

}

Coroutine::~Coroutine()
{

}

WakeupCondition Coroutine::step()
{

}

void Sleep::operator()(uint32_t ms)
{
    m_ms = ms;
    m_state_line = 0;
}

COROUTINE_DEF(Sleep)
{
    COROUTINE_INIT;
    std::cout << "sleep 1" << std::endl;
    await(csleep(m_ms/2));
    std::cout << "sleep 2" << std::endl;
    await(csleep(m_ms/2));
    std::cout << "ret" << std::endl;
    COROUTINE_END;
}


void PeriodicSleep::operator()(uint32_t ms, const char *text)
{
    m_ms = ms;
    m_text = text;
    i = 0;
}

COROUTINE_DEF(PeriodicSleep)
{
    COROUTINE_INIT;
    while (1) {
        call(m_sleep, m_ms);
        std::cout << m_text << ": " << i++ << std::endl;
        yield;
        std::cout << m_text << " post yield" << std::endl;
    }
    COROUTINE_END;
}


WakeupCondition WakeupCondition::finished()
{
    return WakeupCondition{
        .type = WakeupCondition::FINSIHED
    };
}

WakeupCondition WakeupCondition::event(volatile uint8_t *event_bits)
{
    WakeupCondition result;
    result.type = WakeupCondition::EVENT;
    result.event_bits = event_bits;
    return result;
}

WakeupCondition csleep(uint32_t ms)
{
    return WakeupCondition{
        .type = WakeupCondition::TIMER,
        .wakeup_at = sched_clock::now() + std::chrono::milliseconds(ms)
    };
}

WakeupCondition sigint()
{
    sigint_ev = 1;
    return WakeupCondition::event(&sigint_ev);
}

void WaitForNotify::operator()()
{

}

COROUTINE_DEF(WaitForNotify)
{
    COROUTINE_INIT;
    while (1) {
        std::cout << "waiting for notify" << std::endl;
        await(sigint());
        std::cout << "signal received" << std::endl;
    }
    COROUTINE_END;
}

void notify_sigint()
{
    sigint_ev = 0;
}
