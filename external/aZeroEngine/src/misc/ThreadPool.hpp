#include <vector>
#include <future>
#include <queue>

namespace aZero
{
    /// <summary>
    /// Threadpool that can be used for queuing async jobs.
    /// Note: All jobs has to be able to complete.
    /// </summary>
    class ThreadPool
    {
    public:
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        /// <summary>
        /// Job is just an alias for a void function.
        /// </summary>
        using Job = std::function<void()>;

        ThreadPool() = default;

        /// <summary>
        /// Initializes the threadpool with input number of worker threads.
        /// </summary>
        /// <param name="numWorkers">Number of worker threads.</param>
        ThreadPool(uint32_t numWorkers)
        {
            this->Init(numWorkers);
        }

        /// <summary>
        /// Initializes the threadpool with input number of worker threads.
        /// </summary>
        /// <param name="numWorkers">Number of worker threads.</param>
        void Init(uint32_t numWorkers)
        {
            if (m_WorkerThreads.size() == 0)
            {
                m_Shutdown.store(false);
                for (int i = 0; i < numWorkers; i++)
                {
                    m_WorkerThreads.emplace_back([this]
                        {
                            while (true)
                            {
                                std::pair<Job, std::promise<void>> pair;
                                {
                                    std::unique_lock<std::mutex> lock(m_JobQueueLock);
                                    m_JobQueueCV.wait(lock, [this] { return !m_JobQueue.empty() || m_Shutdown.load(); });

                                    if (m_Shutdown.load())
                                    {
                                        break;
                                    }

                                    pair = std::move(m_JobQueue.front());
                                    m_JobQueue.pop();
                                }
                                pair.first();
                                pair.second.set_value();
                            }
                        });
                }
            }
        }

        ~ThreadPool()
        {
            m_Shutdown.store(true);
            m_JobQueueCV.notify_all();
            for (auto& thread : m_WorkerThreads)
            {
                thread.join();
            }
        }

        /// <summary>
        /// Adds a job (void function) to a FIFO queue that will be executed whenever there is an unused thread within the threadpool.
        /// Jobs not yet completed will be skipped on threadpool destruction.
        /// </summary>
        /// <param name="job">The job that will be enqueued.</param>
        /// <returns></returns>
        std::future<void> AddJobA(Job&& job)
        {
            std::promise<void> promise;
            std::future<void> future = promise.get_future();

            {
                std::unique_lock<std::mutex> lock(m_JobQueueLock);
                m_JobQueue.push(std::make_pair(job, std::move(promise)));
            }

            m_JobQueueCV.notify_one();

            return future;
        }

        uint32_t NumWorkerThreads() const { return m_WorkerThreads.size(); }

    private:
        /// <summary>
        /// Active worker threads.
        /// </summary>
        std::vector<std::thread> m_WorkerThreads;

        /// <summary>
        /// Mutex for accessing the job queue.
        /// </summary>
        std::mutex m_JobQueueLock;

        /// <summary>
        /// Conditional variable for notifying threadpool shutdown or new available jobs.
        /// </summary>
        std::condition_variable m_JobQueueCV;

        /// <summary>
        /// FIFO queue of queued jobs.
        /// </summary>
        std::queue<std::pair<Job, std::promise<void>>> m_JobQueue;

        /// <summary>
        /// Atomic to notify threadpool shutdown.
        /// </summary>
        std::atomic_bool m_Shutdown;
    };
}