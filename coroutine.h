#ifndef COROUTINE_H
#define COROUTINE_H

#include <array>
#include <chrono>
#include <cstdint>

typedef std::chrono::steady_clock sched_clock;

#define COROUTINE_INIT         switch(m_state_line) { case 0:
#define COROUTINE_END           } return WakeupCondition::finished();



#define COROUTINE_DECL WakeupCondition step() override
#define COROUTINE_DEF(cls) WakeupCondition cls::step()

#define COROUTINE(name) \
    class name: public Coroutine \
    { \
    private: \
    }; \

#define await(z)     \
        do {\
            m_state_line=__LINE__;\
            return (z); case __LINE__:;\
        } while (0)

#define yield await(WakeupCondition{.type=WakeupCondition::NONE})

#define call(c, ...)\
        do {\
            c(__VA_ARGS__);\
            m_state_line=__LINE__;\
            case __LINE__:;\
            auto condition = c.step();\
            if (condition.type != WakeupCondition::FINSIHED) {\
                return condition;\
            }\
        } while (0);


struct WakeupCondition
{
    enum WakeupConditionType
    {
        FINSIHED = 0,
        NONE = 1,
        TIMER = 2,
        EVENT = 3
    };

    WakeupConditionType type;
    sched_clock::time_point wakeup_at;
    volatile uint8_t *event_bits;

    static WakeupCondition finished();
    static WakeupCondition event(volatile uint8_t *event_bits);
};

class Coroutine
{
public:
    Coroutine();
    virtual ~Coroutine();

protected:
    uint32_t m_state_line;

public:
    virtual WakeupCondition step();
};


class Sleep: public Coroutine
{
private:
    uint32_t m_ms;

public:
    void operator()(uint32_t ms);
    COROUTINE_DECL;
};


class PeriodicSleep: public Coroutine
{
private:
    Sleep m_sleep;
    uint32_t m_ms;
    const char *m_text;

    uint32_t i;

    // Coroutine interface
public:
    void operator()(uint32_t ms, const char *text);
    COROUTINE_DECL;
};


class WaitForNotify: public Coroutine
{
public:
    void operator()();
    COROUTINE_DECL;
};


WakeupCondition csleep(uint32_t ms);
WakeupCondition sigint();
void notify_sigint();


#endif // COROUTINE_H
