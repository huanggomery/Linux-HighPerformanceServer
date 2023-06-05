// 定时器的实现
#include "11/time_wheel_timer.h"

time_wheel::time_wheel()
    :N(60),SI(1),slots(N),cur_slot(0)
{
    /* 空 */
}

time_wheel::time_wheel(int n, int si)
    :N(n),SI(si),slots(N),cur_slot(0)
{
    /* 空 */
}

timer_itr time_wheel::add_timer(int timeout)
{
    if (timeout < 0)
        return slots[0].end();
    
    // 定时器在多少个滴答后触发
    int ticks = timeout < SI ? 1 : timeout / SI;

    // 转动多少圈后触发
    int rotation = ticks / N;

    // 计算定时器应该放在哪个槽中
    int ts = (cur_slot + ticks % N) % N;

    // 在相应位置插入定时器
    slots[ts].push_front(tw_timer(rotation,ts));
    return slots[ts].begin();
}

void time_wheel::del_timer(timer_itr timer)
{
    if (timer == slots[0].end())
        return;
    
    int ts = timer->slot;
    slots[ts].erase(timer);
}

void time_wheel::tick()
{
    auto itr = slots[cur_slot].begin();
    while (itr != slots[cur_slot].end())
    {
        if (itr->rotation > 0)
        {
            --(itr->rotation);
            ++itr;
        }
        else
        {
            itr->cb_func(itr->user_data);
            itr = slots[cur_slot].erase(itr);
        }
    }
    cur_slot = (cur_slot + 1) % N;
}