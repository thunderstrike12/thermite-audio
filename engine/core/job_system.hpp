#pragma once

#include <vector>
#include <future>

namespace std {
class thread;
}

namespace tmt {

class JobManager {
   public:
    JobManager();
    ~JobManager();

    [[nodiscard]] size_t get_thread_count() const { return worker_threads.size(); }

    // A multithreaded parallel for loop.
    void parallel_for(size_t count, const std::function<void(size_t)>& function);

    // Activates the worker threads and makes them spinlock.
    void activate_workers() {
        assert(worker_threads_active.test() == false && "Tried to activate worker threads while they were still active!");
        worker_threads_active.test_and_set();
        worker_threads_active.notify_all();
    }
    // Deactivates the worker threads by making them atomic wait.
    void deactivate_workers() { worker_threads_active.clear(); }

   private:
    // Function looping infinitely on the worker threads.
    void run_worker();
    // Function used to actually execute work, used on both the worker threads and in the main thread.
    void execute_work();

    std::atomic_flag exit;
    std::vector<std::thread> worker_threads;

    std::atomic_flag worker_threads_active;
    std::atomic_flag finished_working;
    size_t total_task_count {0};
    std::atomic<size_t> finished_thread_count {0};
    std::atomic<size_t> next_task_index {0};
    std::function<void(size_t)> task_function;
};

}  // namespace tmt