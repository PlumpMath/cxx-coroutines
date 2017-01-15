#include "scheduler.h"

#include <signal.h>
#include <cstring>

void sigint_handler(int signal)
{
    (void)signal;
    notify_sigint();
}

int main()
{
    struct sigaction sigint_cfg;
    std::memset(&sigint_cfg, 0, sizeof(struct sigaction));
    sigint_cfg.sa_handler = &sigint_handler;

    std::cout << sigaction(SIGUSR1, &sigint_cfg, nullptr) << std::endl;

    PeriodicSleep c1;
    PeriodicSleep c2;
    WaitForNotify c3;

    Scheduler<2> scheduler;
    scheduler.add_task(&c1, 100000, "c1");
    scheduler.add_task(&c3);
    //scheduler.add_task(&c2, 1300, "c2");

    scheduler.run();
    return 0;
}
