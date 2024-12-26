
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <limits>
#include <thread>
#include <atomic>

struct fork
{
    fork() = default;
    fork(fork&) {}
    fork& operator=(const fork&){return *this;}
    fork(fork&&) {}
    fork& operator=(const fork&&){return *this;}

    std::timed_mutex _mtx;
};
struct phil
{
    phil() = default;
    phil(phil&) {}
    phil& operator=(const phil&){return *this;}
    phil(phil&&) {}
    phil& operator=(const phil&&){return *this;}

    void setForks(size_t id, size_t forkLeft, size_t forkRight)
    {
        _id = id;
        _forkLeft = forkLeft;
        _forkRight = forkRight;
    }

    bool tryToLock(std::vector<fork>& forks)
    {
        const auto hungryLevel{hungry()};
        const auto timeToTry{static_cast<size_t>(hungryLevel / 100)};

        if (!forks[_forkLeft]._mtx.try_lock_for(std::chrono::milliseconds{timeToTry}))
        {
            return false;
        }

        if (!forks[_forkRight]._mtx.try_lock_for(std::chrono::milliseconds{timeToTry}))
        {
            forks[_forkLeft]._mtx.unlock();
            return false;
        }
        return true;
    }
    void unlock(std::vector<fork>& forks)
    {
        _eating = false;
        forks[_forkLeft]._mtx.unlock();
        forks[_forkRight]._mtx.unlock();
    }
    void eat()
    {
        _eating = true;
        std::cout << "philosopher " << _id << " forks: (" << _forkLeft << ", " << _forkRight << ") eats after " 
                  << hungry() << " milliseconds of fasting"
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        _lastTimeEaten = std::chrono::system_clock::now();
    }

    double hungry()
    {
        const auto now{std::chrono::system_clock::now()};
        auto duration{std::chrono::duration<double,std::milli>(now - _lastTimeEaten)};
        if (duration.count() > 1000)
        {
            std::cout << "philosopher " << _id << " was starved" << std::endl;
            std::terminate(); 
        }
        return duration.count();
    }

    size_t _id;
    size_t _forkLeft{std::numeric_limits<size_t>::max()}, _forkRight{std::numeric_limits<size_t>::max()};
    std::chrono::time_point<std::chrono::system_clock> _lastTimeEaten{std::chrono::system_clock::now()};
    std::atomic<bool> _eating{false};
};

void printHist(std::vector<size_t>& hist, const std::string& description)
{
    std::cout << description << std::endl;
    std::cout << "-------------------------------------" << std::endl;

    for (auto v : hist)
    {
        for (size_t i = 0 ; i < v ; i++)
        {
            std::cout << '*';
        }
        std::cout << std::endl;
    }
    std::cout << "-------------------------------------" << std::endl << std::endl;
}

template <typename TimeToRun>
bool run(size_t n, TimeToRun t)
{
	std::cout << __FUNCTION__ << " run " << n << " philosophers" << std::endl;
	std::cout << "-------------------------------------------------" << std::endl;

    std::vector<fork> forks; forks.resize(n);
    std::vector<phil> phils; phils.resize(n);

    for (size_t i = 0 ; i < phils.size() ; ++i)
    {
        phils[i].setForks(i, i, (i + n - 1) % n);
    }

    std::vector<size_t> eatHist{};  eatHist.resize(n);
    std::fill(eatHist.begin(), eatHist.end(), 0);

    std::vector<size_t> concurrentEatHist{};  concurrentEatHist.resize(n);

    std::atomic<bool> end{false};
    auto philFunc{[&end, &phils, &forks, &eatHist, &concurrentEatHist](size_t ind){
        auto& phil{phils[ind]};

        while(!end.load())
        {
            if (phil.tryToLock(forks))
            {
                phil.eat();

                // check neighbors
                const auto n{phils.size()};
                const auto rightInd{(ind + 1) % n};
                const auto leftInd{(ind + n - 1) % n};
                if( phils[rightInd]._eating || phils[leftInd]._eating)
                {
                    std::cout << "philosopher " << phil._id << " was eating with it's neighbors: " << leftInd << ", " << rightInd << std::endl;
                    std::terminate();      
                }
                
                // not the best measurement, replace with eating times and then checking intersections
                size_t numOfPhilsEating{0};
                for(const auto& p : phils)
                {
                    if (p._eating)
                    {
                        numOfPhilsEating++;
                    }
                }
                concurrentEatHist[numOfPhilsEating]++;

                phil.unlock(forks);
                eatHist[ind]++;
                std::this_thread::sleep_for(std::chrono::milliseconds{std::min(100 * (ind + 1), 500UL)});
            }
        }
    }};

    std::vector<std::thread> threads;
    for (size_t i = 0 ; i < phils.size() ; ++i)
    {
        threads.emplace_back(philFunc, i);
    }

    std::this_thread::sleep_for(t);
    end.store(true);
    for (auto& t : threads)
    {
        t.join();
    }

    printHist(eatHist, "number of lunches per philosopher");
    printHist(concurrentEatHist, "number of concurrent lunches");

    return true;
}

int main(int /*argc*/, char* /*argv*/[])
{
	if (!run(5, std::chrono::seconds{30}))
		return __LINE__;
	return 0;
}