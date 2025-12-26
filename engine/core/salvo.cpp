#include "salvo.hpp"

#include <latch>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "logger.hpp"

namespace tmt {

Salvo::Salvo() {
    exit.clear();

    // Set the flags to not accidentally start the worker threads once they get created.
    worker_threads_active.clear();
    finished_working.test_and_set();
}

void Salvo::init() {
    // Threads in the pool will be the amount of hardware threads - 2 (to account for main thread and background tasks).
    const size_t thread_count = std::max(std::thread::hardware_concurrency() - 2u, 1u);
    Log::info(Log::Scope::ENGINE, "Starting Salvo job system module: creating {} worker threads", thread_count);
    for (size_t i = 0; i < thread_count; i++) {
        std::thread& worker = worker_threads.emplace_back(&Salvo::run_worker, this);
        SetThreadAffinityMask(worker.native_handle(), 0b1llu << (i + 2));
    }
}

void Salvo::end() {
    // Notify all threads to quit.
    activate_workers();
    exit.test_and_set();

    // Wait for all threads after we notify them to quit.
    Log::info(Log::Scope::ENGINE, "Quiting Salvo job system module: joining worker threads");
    for (std::thread& thread : worker_threads) {
        thread.join();
    }
    deactivate_workers();
}

void Salvo::parallel_for(const size_t count, const std::function<void(size_t)>& function) {
    // Set up the variables used when executing work.
    task_function = function;
    next_task_index = 0;
    total_task_count = count;

    // Make the spin-locking worker threads actually perform the work we just set up.
    finished_working.clear();

    // Activate the worker threads, otherwise we would be running all of "parallel_for" on the main thread, activating them this late in the function can be inefficient so we warn about that.
    bool workers_automatically_woken = false;
    if (!worker_threads_active.test()) {
        workers_automatically_woken = true;
        Log::warn("Worker threads were not activated manually! Threads are now starting up too late!");
        activate_workers();
    }

    // Help out the worker threads by also executing work on the main thread.
    execute_work();

    // If we get here we know that there is no more work left to execute, but some threads might still be performing that work.
    // Waiting on "finished_working" makes sure we only continue when all threads are done performing their part of the work.
    finished_working.wait(false);

    // Disable the worker threads again if we enabled them in this function.
    if (workers_automatically_woken) deactivate_workers();
}

void Salvo::run_worker() {
    while (true) {
        // Wait for the thread to be woken up so that we don't infinitely spin-lock and take up all the PC's CPU time.
        worker_threads_active.wait(false);

        if (exit.test()) return;

        // If we have work then move on to the execute work function, if there is no work but execution gets here it means we are spin-locking just before actual work arrives, and we instead
        // simply give up our time slice using std::this_thread::yield() (Increases performance for other threads while this threads spins).
        if (!finished_working.test()) {
            execute_work();
        } else {
            std::this_thread::yield();
        }
    }
}

void Salvo::execute_work() {
    if (next_task_index >= total_task_count) return;
    ++finished_thread_count;

    // Get the next task index, if its valid perform the work, otherwise continue in this function.
    size_t i = next_task_index.fetch_add(1);
    while (i < total_task_count) {
        task_function(i);

        i = next_task_index.fetch_add(1);
    }

    // If we get here we know that there is no more work left to grab, so this thread checks to see if it was the last thread to finish (fetch_sub(1) returns the value *before* we subtract 1).
    if (finished_thread_count.fetch_sub(1) == 1) {
        // If this is the last worker thread to get here, we know all work is actually done as well, so we notify the main thread.
        finished_working.test_and_set();
        finished_working.notify_one();
    }
}

}  // namespace tmt
