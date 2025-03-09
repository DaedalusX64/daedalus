/*
Copyright (C) 2001 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#pragma once

#ifndef UTILITY_SPINLOCK_H_
#define UTILITY_SPINLOCK_H_

#include <thread>

#include <atomic>
#include <thread>

class CSpinLock
{
public:
    explicit CSpinLock(std::atomic<u32>* var)
        : Var(var)
    {
        // Try to acquire the lock using an atomic compare-and-swap (CAS)
        while (Var->load(std::memory_order_acquire) != 0) {
            std::this_thread::yield();  // Yield to avoid busy-waiting
        }
        // CAS operation: atomically set the value to 1 if it was 0
        Var->store(1, std::memory_order_release);
    }

    ~CSpinLock()
    {
        // Release the lock
        Var->store(0, std::memory_order_release);
    }

private:
    std::atomic<u32>* Var;
};


#endif // UTILITY_SPINLOCK_H_
