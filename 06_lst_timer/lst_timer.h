#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include <iostream>
#include <netinet/in.h>

using namespace std;

#define BUFFER_SIZE 64
class util_timer;
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[ BUFFER_SIZE ];
    util_timer* timer;
};

class util_timer
{
public:
    util_timer() : prev( NULL ), next( NULL ){}

public:
   time_t expire; 
   void (*cb_func)( client_data* );
   client_data* user_data;
   util_timer* prev;
   util_timer* next;
};

class sort_timer_lst
{
public:
    sort_timer_lst() : head( NULL ), tail( NULL ) {}
    ~sort_timer_lst()
    {
        util_timer* tmp = head;
        while( tmp )
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
    void add_timer( util_timer* timer )
    {
        if( !timer )
        {
            return;
        }
        //如果当前链表为空，则设置为头尾节点
        if( !head )
        {
            head = tail = timer;
            return; 
        }
        //如果时间比头节点的都要短，那么放在头部
        if( timer->expire < head->expire )
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        //否则插入到后面合适的位置
        add_timer( timer, head );
    }
    void adjust_timer( util_timer* timer )
    {
        if( !timer )
        {
            return;
        }
        util_timer* tmp = timer->next;
        if( !tmp || ( timer->expire < tmp->expire ) )
        {
            return;
        }
        //如果要调整的是头部，那么拿出头部节点，并插入到后面合适的位置
        if( timer == head )
        {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer( timer, head );
        }
        else
        {
            //否则将这个节点从链表中取出，重新接起来，再插入到后面合适的位置
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer( timer, timer->next );
        }
    }
    void del_timer( util_timer* timer )
    {
        if( !timer )
        {
            return;
        }
        //如果当前链表中只有一个节点
        if( ( timer == head ) && ( timer == tail ) )
        {
            delete timer;
            head = NULL;
            tail = NULL;
            return;
        }
        //如果是头节点
        if( timer == head )
        {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }
        //如果是尾节点
        if( timer == tail )
        {
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }
        //如果是中间节点
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    //到定时的时间时，从前往后挨着检查超时的定时器，删除
    void tick()
    {
        if( !head )
        {
            return;
        }

        cout << "timer tick" << endl;
        time_t cur = time( NULL );
        util_timer* tmp = head;
        while( tmp )
        {
            if( cur < tmp->expire )
            {
                break;
            }
            tmp->cb_func( tmp->user_data );
            head = tmp->next;
            if( head )
            {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    void add_timer( util_timer* timer, util_timer* lst_head )
    {
        util_timer* prev = lst_head;
        util_timer* tmp = prev->next;
        while( tmp )
        {
            if( timer->expire < tmp->expire )
            {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        if( !tmp )
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
        
    }

private:
    util_timer* head;
    util_timer* tail;
};

#endif