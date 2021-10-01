#include <system/mutex/mutex.hpp>

void mutex_lock(mutex *mutex)
{
    while (!__sync_bool_compare_and_swap(&mutex->locked, false, true));
}

void mutex_unlock(mutex *mutex)
{
    __sync_bool_compare_and_swap(&mutex->locked, true, false);
}
