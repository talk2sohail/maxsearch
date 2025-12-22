#ifndef SEARCHENGINE_H
#define SEARCHENGINE_H

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <CoreServices/CoreServices.h>

class SearchEngine {
public:
    SearchEngine();
    ~SearchEngine();

    // The UI calls this whenever text changes
    void Search(const std::string& query);

    // The UI calls this every frame to get the latest results
    std::vector<std::string> GetResults();

private:
    void SearchWorker();
    
    // CoreFoundation RAII helpers
    std::string CFStringToString(CFStringRef cfStr);
    
    // State
    std::atomic<bool> isRunning;
    bool needsSearch; // Changed from atomic to bool as it's protected by mutex
    std::string currentQuery;
    std::mutex queryMutex;
    std::condition_variable cv; // Signal for new work

    std::vector<std::string> currentResults;
    std::mutex resultsMutex;
    
    std::thread workerThread;
};

#endif
