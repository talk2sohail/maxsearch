#include "SearchEngine.h"
#include <algorithm>

// Helper to auto-release CF types
template <typename T> struct CFGuard {
    T obj;
    CFGuard(T o) : obj(o) {}
    ~CFGuard() {
        if (obj)
            CFRelease(obj);
    }
    operator T() {
        return obj;
    }
};

SearchEngine::SearchEngine() : isRunning(true), needsSearch(false) {
    workerThread = std::thread(&SearchEngine::SearchWorker, this);
}

SearchEngine::~SearchEngine() {
    {
        std::lock_guard<std::mutex> lock(queryMutex);
        isRunning = false;
        needsSearch = true; // Force wake up
    }
    cv.notify_one();

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void SearchEngine::Search(const std::string &query) {
    {
        std::lock_guard<std::mutex> lock(queryMutex);
        currentQuery = query;
        needsSearch = true;
    }
    cv.notify_one();
}

std::vector<std::string> SearchEngine::GetResults() {
    std::lock_guard<std::mutex> lock(resultsMutex);
    return currentResults;
}

std::string SearchEngine::CFStringToString(CFStringRef cfStr) {
    if (!cfStr)
        return "";

    // Try to get direct pointer
    const char *cPtr = CFStringGetCStringPtr(cfStr, kCFStringEncodingUTF8);
    if (cPtr)
        return std::string(cPtr);

    // Fallback: Copy to buffer
    CFIndex length = CFStringGetLength(cfStr);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    std::vector<char> buffer(maxSize);

    if (CFStringGetCString(cfStr, buffer.data(), maxSize, kCFStringEncodingUTF8)) {
        return std::string(buffer.data());
    }
    return "";
}

void SearchEngine::SearchWorker() {
    while (isRunning) {
        std::string localQuery;

        // 1. Wait for work
        {
            std::unique_lock<std::mutex> lock(queryMutex);
            cv.wait(lock, [this] { return needsSearch || !isRunning; });

            if (!isRunning)
                return;

            localQuery = currentQuery;
            needsSearch = false;
        }

        if (localQuery.empty()) {
            std::lock_guard<std::mutex> resLock(resultsMutex);
            currentResults.clear();
            continue;
        }

        // 2. Prepare the Query String for MDQuery
        // Format: kMDItemDisplayName == "query*"c
        // "c" means case-insensitive
        // We use Prefix match (query*) instead of Contains (*query*) for performance
        std::string queryString = "kMDItemDisplayName == \"" + localQuery + "*\"c";

        CFGuard<CFStringRef> cfQueryString = CFStringCreateWithCString(
            kCFAllocatorDefault, queryString.c_str(), kCFStringEncodingUTF8);

        // 3. Create and Execute Query
        CFGuard<MDQueryRef> query =
            MDQueryCreate(kCFAllocatorDefault, cfQueryString.obj, NULL, NULL);
        if (!query.obj)
            continue;

        // Limit scope to Local Volumes (avoid network drives)
        // MDQuerySetSearchScope(query.obj, kMDQueryScopeComputer, 0); // Deprecated in modern
        // macOS? Let's rely on default scope for now which is usually Home + indexed drives.

        // Execute Synchronously (since we are already in a worker thread)
        // This blocks until search is "done" or initial batch is ready?
        // Actually MDQueryExecute is blocking for the *initial* gathering if we don't set a
        // callback. But for "Instant" results, we can just run it. Note: MDQueryExecute acts async
        // usually. We need to wait a tiny bit or loop? Better approach for synchronous one-shot in
        // thread: MDQueryExecute(query, kMDQuerySynchronous)

        if (MDQueryExecute(query.obj, kMDQuerySynchronous)) {
            // 4. Gather Results
            CFIndex count = MDQueryGetResultCount(query.obj);
            // Fetch more candidates (50) to ensure apps bubble up, even if ranked lower by default
            if (count > 50)
                count = 50;

            std::vector<std::string> tempResults;
            tempResults.reserve(count);

            for (CFIndex i = 0; i < count; i++) {
                // Get the Item
                MDItemRef item = (MDItemRef)MDQueryGetResultAtIndex(query.obj, i);
                if (!item)
                    continue;

                // Get the Path
                CFGuard<CFStringRef> path = (CFStringRef)MDItemCopyAttribute(item, kMDItemPath);
                if (path.obj) {
                    tempResults.push_back(CFStringToString(path.obj));
                }
            }

            // Sort: Apps first, then by length (shorter = likely more relevant)
            std::stable_sort(tempResults.begin(), tempResults.end(),
                             [](const std::string &a, const std::string &b) {
                                 bool aIsApp =
                                     (a.length() > 4 && a.substr(a.length() - 4) == ".app");
                                 bool bIsApp =
                                     (b.length() > 4 && b.substr(b.length() - 4) == ".app");

                                 if (aIsApp != bIsApp)
                                     return aIsApp;              // Apps come first
                                 return a.length() < b.length(); // Shorter paths come next
                             });

            // Trim to top 10 for display
            if (tempResults.size() > 10) {
                tempResults.resize(10);
            }

            // 5. Publish Results
            {
                std::lock_guard<std::mutex> resLock(resultsMutex);
                currentResults = tempResults;
            }
        }
    }
}